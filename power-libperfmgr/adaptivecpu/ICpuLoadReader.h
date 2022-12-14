#pragma once

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

#include <array>
#include <sstream>

#include "Model.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

class ICpuLoadReader {
  public:
    // Initialize reading, must be done before calling other methods.
    // Work is not done in constructor as it accesses files.
    virtual bool Init() = 0;
    // Get the load of each CPU, since the last time this method was called.
    virtual bool GetRecentCpuLoads(
            std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage) = 0;
    // Dump internal state to a string stream. Used for dumpsys.
    virtual void DumpToStream(std::stringstream &stream) const = 0;
    virtual ~ICpuLoadReader() {}
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
