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

#define LOG_TAG "powerhal-libperfmgr"
#define ATRACE_TAG (ATRACE_TAG_POWER | ATRACE_TAG_HAL)

#include "Device.h"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <utils/Trace.h>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

Device ReadDevice() {
    ATRACE_CALL();
    const std::string deviceProperty = ::android::base::GetProperty("ro.product.device", "");
    Device device;
    if (deviceProperty == "raven") {
        device = Device::RAVEN;
    } else if (deviceProperty == "oriole") {
        device = Device::ORIOLE;
    } else {
        LOG(WARNING) << "Failed to parse device property, setting to UNKNOWN: " << deviceProperty;
        device = Device::UNKNOWN;
    }
    LOG(DEBUG) << "Parsed device: deviceProperty=" << deviceProperty
               << ", device=" << static_cast<uint32_t>(device);
    return device;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
