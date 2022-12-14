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

#include <chrono>
#include <map>

#include "ICpuLoadReader.h"
#include "IFilesystem.h"
#include "ITimeSource.h"
#include "Model.h"
#include "RealFilesystem.h"
#include "TimeSource.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

struct CpuTime {
    std::chrono::microseconds idleTime;
    std::chrono::microseconds totalTime;
};

// Reads CPU idle stats from /sys/devices/system/cpu/cpuN/cpuidle.
class CpuLoadReaderSysDevices : public ICpuLoadReader {
  public:
    CpuLoadReaderSysDevices()
        : mFilesystem(std::make_unique<RealFilesystem>()),
          mTimeSource(std::make_unique<TimeSource>()) {}
    CpuLoadReaderSysDevices(std::unique_ptr<IFilesystem> filesystem,
                            std::unique_ptr<ITimeSource> timeSource)
        : mFilesystem(std::move(filesystem)), mTimeSource(std::move(timeSource)) {}

    bool Init() override;
    bool GetRecentCpuLoads(std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage) override;
    void DumpToStream(std::stringstream &stream) const override;

  private:
    const std::unique_ptr<IFilesystem> mFilesystem;
    const std::unique_ptr<ITimeSource> mTimeSource;

    std::array<CpuTime, NUM_CPU_CORES> mPreviousCpuTimes;
    std::vector<std::string> mIdleStateNames;

    bool ReadCpuTimes(std::array<CpuTime, NUM_CPU_CORES> *result) const;
    bool ReadIdleStateNames(std::vector<std::string> *result) const;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
