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

#include <string>
#include <unordered_map>
#include <vector>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

enum class ThrottleDecision {
    NO_THROTTLE = 0,
    THROTTLE_50 = 1,
    THROTTLE_60 = 2,
    THROTTLE_70 = 3,
    THROTTLE_80 = 4,
    THROTTLE_90 = 5,
    FIRST = NO_THROTTLE,
    LAST = THROTTLE_90,
};

std::string ThrottleString(ThrottleDecision throttleDecision);

static const std::unordered_map<ThrottleDecision, std::vector<std::string>>
        THROTTLE_DECISION_TO_HINT_NAMES = {
                {ThrottleDecision::NO_THROTTLE, {}},
                {ThrottleDecision::THROTTLE_50,
                 {"LOW_POWER_LITTLE_CLUSTER_50", "LOW_POWER_MID_CLUSTER_50", "LOW_POWER_CPU_50"}},
                {ThrottleDecision::THROTTLE_60,
                 {"LOW_POWER_LITTLE_CLUSTER_60", "LOW_POWER_MID_CLUSTER_60", "LOW_POWER_CPU_60"}},
                {ThrottleDecision::THROTTLE_70,
                 {"LOW_POWER_LITTLE_CLUSTER_70", "LOW_POWER_MID_CLUSTER_70", "LOW_POWER_CPU_70"}},
                {ThrottleDecision::THROTTLE_80,
                 {"LOW_POWER_LITTLE_CLUSTER_80", "LOW_POWER_MID_CLUSTER_80", "LOW_POWER_CPU_80"}},
                {ThrottleDecision::THROTTLE_90,
                 {"LOW_POWER_LITTLE_CLUSTER_90", "LOW_POWER_MID_CLUSTER_90", "LOW_POWER_CPU_90"}}};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
