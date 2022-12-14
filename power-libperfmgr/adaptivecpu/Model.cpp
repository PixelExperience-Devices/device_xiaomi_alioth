/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include "Model.h"

#include <android-base/logging.h>
#include <utils/Trace.h>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

bool ModelInput::SetCpuFreqiencies(
        const std::vector<CpuPolicyAverageFrequency> &cpuPolicyAverageFrequencies) {
    ATRACE_CALL();
    if (cpuPolicyAverageFrequencies.size() != cpuPolicyAverageFrequencyHz.size()) {
        LOG(ERROR) << "Received incorrect amount of CPU policy frequencies, expected "
                   << cpuPolicyAverageFrequencyHz.size() << ", received "
                   << cpuPolicyAverageFrequencies.size();
        return false;
    }
    int32_t previousPolicyId = -1;
    for (uint32_t i = 0; i < cpuPolicyAverageFrequencies.size(); i++) {
        if (previousPolicyId >= static_cast<int32_t>(cpuPolicyAverageFrequencies[i].policyId)) {
            LOG(ERROR) << "CPU frequencies weren't sorted by policy ID, found " << previousPolicyId
                       << " " << cpuPolicyAverageFrequencies[i].policyId;
            return false;
        }
        previousPolicyId = cpuPolicyAverageFrequencies[i].policyId;
        cpuPolicyAverageFrequencyHz[i] = cpuPolicyAverageFrequencies[i].averageFrequencyHz;
    }
    return true;
}

void ModelInput::LogToAtrace() const {
    if (!ATRACE_ENABLED()) {
        return;
    }
    ATRACE_CALL();
    for (int i = 0; i < cpuPolicyAverageFrequencyHz.size(); i++) {
        ATRACE_INT((std::string("ModelInput_frequency_") + std::to_string(i)).c_str(),
                   static_cast<int>(cpuPolicyAverageFrequencyHz[i]));
    }
    for (int i = 0; i < cpuCoreIdleTimesPercentage.size(); i++) {
        ATRACE_INT((std::string("ModelInput_idle_") + std::to_string(i)).c_str(),
                   static_cast<int>(cpuCoreIdleTimesPercentage[i] * 100));
    }
    ATRACE_INT("ModelInput_workDurations_averageDurationNs",
               workDurationFeatures.averageDuration.count());
    ATRACE_INT("ModelInput_workDurations_maxDurationNs", workDurationFeatures.maxDuration.count());
    ATRACE_INT("ModelInput_workDurations_numMissedDeadlines",
               workDurationFeatures.numMissedDeadlines);
    ATRACE_INT("ModelInput_workDurations_numDurations", workDurationFeatures.numDurations);
    ATRACE_INT("ModelInput_prevThrottle", (int)previousThrottleDecision);
    ATRACE_INT("ModelInput_device", static_cast<int>(device));
}

ThrottleDecision Model::Run(const std::deque<ModelInput> &modelInputs,
                            const AdaptiveCpuConfig &config) {
    ATRACE_CALL();
    if (config.randomThrottleDecisionProbability > 0 &&
        mShouldRandomThrottleDistribution(mGenerator) < config.randomThrottleDecisionProbability) {
        std::uniform_int_distribution<uint32_t> optionDistribution(
                0, config.randomThrottleOptions.size() - 1);
        const ThrottleDecision throttleDecision =
                config.randomThrottleOptions[optionDistribution(mGenerator)];
        LOG(VERBOSE) << "Randomly overrided throttle decision: "
                     << static_cast<uint32_t>(throttleDecision);
        ATRACE_INT("AdaptiveCpu_randomThrottleDecision", static_cast<uint32_t>(throttleDecision));
        return throttleDecision;
    }
    ATRACE_INT("AdaptiveCpu_randomThrottleDecision", -1);
    return RunDecisionTree(modelInputs);
}

ThrottleDecision Model::RunDecisionTree(const std::deque<ModelInput> &modelInputs
                                        __attribute__((unused))) {
    ATRACE_CALL();
#include "models/model.inc"
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
