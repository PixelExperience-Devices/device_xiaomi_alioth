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

#include "ThrottleDecision.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

std::string ThrottleString(ThrottleDecision throttleDecision) {
    switch (throttleDecision) {
        case ThrottleDecision::NO_THROTTLE:
            return "NO_THROTTLE";
        case ThrottleDecision::THROTTLE_60:
            return "THROTTLE_60";
        case ThrottleDecision::THROTTLE_70:
            return "THROTTLE_70";
        case ThrottleDecision::THROTTLE_80:
            return "THROTTLE_80";
        case ThrottleDecision::THROTTLE_90:
            return "THROTTLE_90";
        default:
            return "unknown";
    }
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
