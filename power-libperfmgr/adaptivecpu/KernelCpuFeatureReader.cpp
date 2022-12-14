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

#include "KernelCpuFeatureReader.h"

#include <android-base/logging.h>
#include <utils/Trace.h>

#include <ostream>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

constexpr std::string_view kKernelFilePath("/proc/vendor_sched/acpu_stats");
constexpr size_t kReadBufferSize = sizeof(acpu_stats) * NUM_CPU_CORES;

bool KernelCpuFeatureReader::Init() {
    ATRACE_CALL();
    if (!OpenStatsFile(&mStatsFile)) {
        return false;
    }
    return ReadStats(&mPreviousStats, &mPreviousReadTime);
}

bool KernelCpuFeatureReader::GetRecentCpuFeatures(
        std::array<double, NUM_CPU_POLICIES> *cpuPolicyAverageFrequencyHz,
        std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage) {
    ATRACE_CALL();
    std::array<acpu_stats, NUM_CPU_CORES> stats;
    std::chrono::nanoseconds readTime;
    if (!ReadStats(&stats, &readTime)) {
        return false;
    }
    const std::chrono::nanoseconds timeDelta = readTime - mPreviousReadTime;

    for (size_t i = 0; i < NUM_CPU_POLICIES; i++) {
        // acpu_stats has data per-CPU, but frequency data is equivalent for all CPUs in a policy.
        // So, we only read the first CPU in each policy.
        const size_t statsIdx = CPU_POLICY_INDICES[i];
        if (stats[statsIdx].weighted_sum_freq < mPreviousStats[statsIdx].weighted_sum_freq) {
            LOG(WARNING) << "New weighted_sum_freq is less than old: new="
                         << stats[statsIdx].weighted_sum_freq
                         << ", old=" << mPreviousStats[statsIdx].weighted_sum_freq;
            mPreviousStats[statsIdx].weighted_sum_freq = stats[statsIdx].weighted_sum_freq;
        }
        (*cpuPolicyAverageFrequencyHz)[i] =
                static_cast<double>(stats[statsIdx].weighted_sum_freq -
                                    mPreviousStats[statsIdx].weighted_sum_freq) /
                timeDelta.count();
    }
    for (size_t i = 0; i < NUM_CPU_CORES; i++) {
        if (stats[i].total_idle_time_ns < mPreviousStats[i].total_idle_time_ns) {
            LOG(WARNING) << "New total_idle_time_ns is less than old: new="
                         << stats[i].total_idle_time_ns
                         << ", old=" << mPreviousStats[i].total_idle_time_ns;
            mPreviousStats[i].total_idle_time_ns = stats[i].total_idle_time_ns;
        }
        (*cpuCoreIdleTimesPercentage)[i] =
                static_cast<double>(stats[i].total_idle_time_ns -
                                    mPreviousStats[i].total_idle_time_ns) /
                timeDelta.count();
    }

    mPreviousStats = stats;
    mPreviousReadTime = readTime;
    return true;
}

bool KernelCpuFeatureReader::OpenStatsFile(std::unique_ptr<std::istream> *file) {
    ATRACE_CALL();
    return mFilesystem->ReadFileStream(kKernelFilePath.data(), file);
}

bool KernelCpuFeatureReader::ReadStats(std::array<acpu_stats, NUM_CPU_CORES> *stats,
                                       std::chrono::nanoseconds *readTime) {
    ATRACE_CALL();
    *readTime = mTimeSource->GetKernelTime();
    if (!mFilesystem->ResetFileStream(mStatsFile)) {
        return false;
    }
    char buffer[kReadBufferSize];
    {
        ATRACE_NAME("read");
        if (!mStatsFile->read(buffer, kReadBufferSize).good()) {
            LOG(ERROR) << "Failed to read stats file";
            return false;
        }
    }
    const size_t bytesRead = mStatsFile->gcount();
    if (bytesRead != kReadBufferSize) {
        LOG(ERROR) << "Didn't read full data: expected=" << kReadBufferSize
                   << ", actual=" << bytesRead;
        return false;
    }
    const auto kernelStructs = reinterpret_cast<acpu_stats *>(buffer);
    std::copy(kernelStructs, kernelStructs + NUM_CPU_CORES, stats->begin());
    return true;
}

void KernelCpuFeatureReader::DumpToStream(std::ostream &stream) const {
    ATRACE_CALL();
    stream << "CPU features from acpu_stats:\n";
    for (size_t i = 0; i < NUM_CPU_CORES; i++) {
        stream << "- CPU " << i << ": weighted_sum_freq=" << mPreviousStats[i].weighted_sum_freq
               << ", total_idle_time_ns=" << mPreviousStats[i].total_idle_time_ns << "\n";
    }
    stream << "- Last read time: " << mPreviousReadTime.count() << "ns\n";
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
