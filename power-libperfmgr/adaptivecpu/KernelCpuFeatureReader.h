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

#include <array>
#include <ostream>

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

// Kernel <-> Userspace ABI for CPU features. See kernel/sched/acpu.c.
// Contains CPU statistics for a single CPU. The kernel reports an `acpu_stats` struct for each CPU
// on the system.
struct acpu_stats {
    // Sum of the CPU frequencies that the CPU used, multiplied by how much time was spent in each
    // frequency. Measured in ns*KHz. E.g.:
    //   10ns at 100MHz, 2ns at 50MHz = 10*100,000 + 2*50,000 = 1,100,000
    // This is used to calculate the average frequency the CPU was running at between two times:
    //   (new.weighted_sum_freq - old.weighted_sum_freq) / elapsed_time_ns
    uint64_t weighted_sum_freq;
    // The total time (in nanoseconds) that the CPU was idle.
    // This is ued to calculate the percent of time the CPU was idle between two times:
    //   (new.total_idle_time_ns - old.total_idle_time_ns) / elapsed_time_ns
    uint64_t total_idle_time_ns;
};

class KernelCpuFeatureReader {
  public:
    KernelCpuFeatureReader()
        : mFilesystem(std::make_unique<RealFilesystem>()),
          mTimeSource(std::make_unique<TimeSource>()) {}
    KernelCpuFeatureReader(std::unique_ptr<IFilesystem> filesystem,
                           std::unique_ptr<ITimeSource> timeSource)
        : mFilesystem(std::move(filesystem)), mTimeSource(std::move(timeSource)) {}

    bool Init();
    bool GetRecentCpuFeatures(std::array<double, NUM_CPU_POLICIES> *cpuPolicyAverageFrequencyHz,
                              std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage);
    void DumpToStream(std::ostream &stream) const;

  private:
    const std::unique_ptr<IFilesystem> mFilesystem;
    const std::unique_ptr<ITimeSource> mTimeSource;
    // We only open the stats file once and reuse the file descriptor. We find this reduces
    // ReadStats runtime by 2x.
    std::unique_ptr<std::istream> mStatsFile;
    std::array<acpu_stats, NUM_CPU_CORES> mPreviousStats;
    std::chrono::nanoseconds mPreviousReadTime;
    bool OpenStatsFile(std::unique_ptr<std::istream> *file);
    bool ReadStats(std::array<acpu_stats, NUM_CPU_CORES> *stats,
                   std::chrono::nanoseconds *readTime);
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
