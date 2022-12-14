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

#pragma once

#include <chrono>
#include <iostream>

#include "ThrottleDecision.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

struct AdaptiveCpuConfig {
    static bool ReadFromSystemProperties(AdaptiveCpuConfig *output);
    static const AdaptiveCpuConfig DEFAULT;

    // How long to sleep for between Adaptive CPU runs.
    std::chrono::milliseconds iterationSleepDuration;
    // Timeout applied to hints. If Adaptive CPU doesn't receive any frames in this time, CPU
    // throttling hints are cancelled.
    std::chrono::milliseconds hintTimeout;
    // Instead of throttling based on model output, choose a random throttle X% of the time. Must be
    // between 0 and 1 inclusive.
    double randomThrottleDecisionProbability;
    std::vector<ThrottleDecision> randomThrottleOptions;
    // Setting AdaptiveCpu to enabled only lasts this long. For a continuous run, AdaptiveCpu needs
    // to receive the enabled hint more frequently than this value.
    std::chrono::milliseconds enabledHintTimeout;

    bool operator==(const AdaptiveCpuConfig &other) const;
};

std::ostream &operator<<(std::ostream &os, const AdaptiveCpuConfig &config);

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
