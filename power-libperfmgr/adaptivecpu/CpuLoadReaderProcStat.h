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

#include <map>

#include "ICpuLoadReader.h"
#include "IFilesystem.h"
#include "RealFilesystem.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

struct CpuTime {
    uint64_t idleTimeMs;
    uint64_t totalTimeMs;
};

// Reads CPU idle stats from /proc/stat.
class CpuLoadReaderProcStat : public ICpuLoadReader {
  public:
    CpuLoadReaderProcStat() : mFilesystem(std::make_unique<RealFilesystem>()) {}
    CpuLoadReaderProcStat(std::unique_ptr<IFilesystem> filesystem)
        : mFilesystem(std::move(filesystem)) {}

    bool Init() override;
    bool GetRecentCpuLoads(std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage) override;
    void DumpToStream(std::stringstream &stream) const override;

  private:
    std::map<uint32_t, CpuTime> mPreviousCpuTimes;
    const std::unique_ptr<IFilesystem> mFilesystem;

    bool ReadCpuTimes(std::map<uint32_t, CpuTime> *result);
    // Converts jiffies to milliseconds. Jiffies is the granularity the kernel reports times in,
    // including the timings in CPU statistics.
    static uint64_t JiffiesToMs(uint64_t jiffies);
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
