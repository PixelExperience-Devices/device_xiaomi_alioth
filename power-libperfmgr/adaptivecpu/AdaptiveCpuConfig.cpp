/*
 * Copyright (C) 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "powerhal-adaptivecpu"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "AdaptiveCpuConfig.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <inttypes.h>
#include <utils/Trace.h>

#include <sstream>
#include <string>
#include <string_view>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""min;

constexpr std::string_view kIterationSleepDurationProperty(
        "debug.adaptivecpu.iteration_sleep_duration_ms");
static const std::chrono::milliseconds kIterationSleepDurationMin = 20ms;
constexpr std::string_view kHintTimeoutProperty("debug.adaptivecpu.hint_timeout_ms");
// "percent" as range is 0-100, while the in-memory is "probability" as range is 0-1.
constexpr std::string_view kRandomThrottleDecisionPercentProperty(
        "debug.adaptivecpu.random_throttle_decision_percent");
constexpr std::string_view kRandomThrottleOptionsProperty(
        "debug.adaptivecpu.random_throttle_options");
constexpr std::string_view kEnabledHintTimeoutProperty("debug.adaptivecpu.enabled_hint_timeout_ms");

bool ParseThrottleDecisions(const std::string &input, std::vector<ThrottleDecision> *output);
std::string FormatThrottleDecisions(const std::vector<ThrottleDecision> &throttleDecisions);

const AdaptiveCpuConfig AdaptiveCpuConfig::DEFAULT{
        // N.B.: The model will typically be trained with this value set to 25ms. We set it to 1s as
        // a safety measure, but best performance will be seen at 25ms.
        .iterationSleepDuration = 1000ms,
        .hintTimeout = 2000ms,
        .randomThrottleDecisionProbability = 0,
        .randomThrottleOptions = {ThrottleDecision::NO_THROTTLE, ThrottleDecision::THROTTLE_50,
                                  ThrottleDecision::THROTTLE_60, ThrottleDecision::THROTTLE_70,
                                  ThrottleDecision::THROTTLE_80, ThrottleDecision::THROTTLE_90},
        .enabledHintTimeout = 120min,
};

bool AdaptiveCpuConfig::ReadFromSystemProperties(AdaptiveCpuConfig *output) {
    ATRACE_CALL();

    output->iterationSleepDuration = std::chrono::milliseconds(
            ::android::base::GetUintProperty<uint32_t>(kIterationSleepDurationProperty.data(),
                                                       DEFAULT.iterationSleepDuration.count()));
    output->iterationSleepDuration =
            std::max(output->iterationSleepDuration, kIterationSleepDurationMin);

    output->hintTimeout = std::chrono::milliseconds(::android::base::GetUintProperty<uint32_t>(
            kHintTimeoutProperty.data(), DEFAULT.hintTimeout.count()));

    output->randomThrottleDecisionProbability =
            static_cast<double>(::android::base::GetUintProperty<uint32_t>(
                    kRandomThrottleDecisionPercentProperty.data(),
                    DEFAULT.randomThrottleDecisionProbability * 100)) /
            100;
    if (output->randomThrottleDecisionProbability > 1.0) {
        LOG(ERROR) << "Received bad value for " << kRandomThrottleDecisionPercentProperty << ": "
                   << output->randomThrottleDecisionProbability;
        return false;
    }

    const std::string randomThrottleOptionsStr =
            ::android::base::GetProperty(kRandomThrottleOptionsProperty.data(),
                                         FormatThrottleDecisions(DEFAULT.randomThrottleOptions));
    output->randomThrottleOptions.clear();
    if (!ParseThrottleDecisions(randomThrottleOptionsStr, &output->randomThrottleOptions)) {
        return false;
    }

    output->enabledHintTimeout =
            std::chrono::milliseconds(::android::base::GetUintProperty<uint32_t>(
                    kEnabledHintTimeoutProperty.data(), DEFAULT.enabledHintTimeout.count()));

    return true;
}

bool AdaptiveCpuConfig::operator==(const AdaptiveCpuConfig &other) const {
    return iterationSleepDuration == other.iterationSleepDuration &&
           hintTimeout == other.hintTimeout &&
           randomThrottleDecisionProbability == other.randomThrottleDecisionProbability &&
           enabledHintTimeout == other.enabledHintTimeout &&
           randomThrottleOptions == other.randomThrottleOptions;
}

std::ostream &operator<<(std::ostream &stream, const AdaptiveCpuConfig &config) {
    stream << "AdaptiveCpuConfig(";
    stream << "iterationSleepDuration=" << config.iterationSleepDuration.count() << "ms, ";
    stream << "hintTimeout=" << config.hintTimeout.count() << "ms, ";
    stream << "randomThrottleDecisionProbability=" << config.randomThrottleDecisionProbability
           << ", ";
    stream << "enabledHintTimeout=" << config.enabledHintTimeout.count() << "ms, ";
    stream << "randomThrottleOptions=[" << FormatThrottleDecisions(config.randomThrottleOptions)
           << "]";
    stream << ")";
    return stream;
}

bool ParseThrottleDecisions(const std::string &input, std::vector<ThrottleDecision> *output) {
    std::stringstream ss(input);
    while (ss.good()) {
        std::string throttleDecisionStr;
        if (std::getline(ss, throttleDecisionStr, ',').fail()) {
            LOG(ERROR) << "Failed to getline on throttle decisions string: " << input;
            return false;
        }
        uint32_t throttleDecisionInt;
        int scanEnd;
        if (std::sscanf(throttleDecisionStr.c_str(), "%" PRIu32 "%n", &throttleDecisionInt,
                        &scanEnd) != 1 ||
            scanEnd != throttleDecisionStr.size()) {
            LOG(ERROR) << "Failed to parse as int: str=" << throttleDecisionStr
                       << ", input=" << input << ", scanEnd=" << scanEnd;
            return false;
        }
        if (throttleDecisionInt < static_cast<uint32_t>(ThrottleDecision::FIRST) ||
            throttleDecisionInt > static_cast<uint32_t>(ThrottleDecision::LAST)) {
            LOG(ERROR) << "Failed to parse throttle decision: throttleDecision="
                       << throttleDecisionInt << ", input=" << input;
            return false;
        }
        output->push_back(static_cast<ThrottleDecision>(throttleDecisionInt));
    }
    if (output->empty()) {
        LOG(ERROR) << "Failed to find any throttle decisions, must have at least one: " << input;
        return false;
    }
    return true;
}

std::string FormatThrottleDecisions(const std::vector<ThrottleDecision> &throttleDecisions) {
    std::stringstream ss;
    for (size_t i = 0; i < throttleDecisions.size(); i++) {
        ss << static_cast<uint32_t>(throttleDecisions[i]);
        if (i < throttleDecisions.size() - 1) {
            ss << ",";
        }
    }
    return ss.str();
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
