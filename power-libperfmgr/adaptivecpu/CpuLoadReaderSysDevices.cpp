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

#include "CpuLoadReaderSysDevices.h"

#include <android-base/logging.h>
#include <inttypes.h>
#include <utils/Trace.h>

#include <fstream>
#include <sstream>
#include <string>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

std::chrono::nanoseconds getKernelTime() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return std::chrono::nanoseconds(ts.tv_sec * 1000000000UL + ts.tv_nsec);
}

bool CpuLoadReaderSysDevices::Init() {
    mIdleStateNames.clear();
    if (!ReadIdleStateNames(&mIdleStateNames)) {
        return false;
    }
    return ReadCpuTimes(&mPreviousCpuTimes);
}

bool CpuLoadReaderSysDevices::GetRecentCpuLoads(
        std::array<double, NUM_CPU_CORES> *cpuCoreIdleTimesPercentage) {
    ATRACE_CALL();
    if (cpuCoreIdleTimesPercentage == nullptr) {
        LOG(ERROR) << "Got nullptr output in getRecentCpuLoads";
        return false;
    }
    std::array<CpuTime, NUM_CPU_CORES> cpuTimes;
    if (!ReadCpuTimes(&cpuTimes)) {
        return false;
    }
    if (cpuTimes.empty()) {
        LOG(ERROR) << "Failed to find any CPU times";
        return false;
    }
    for (size_t cpuId = 0; cpuId < NUM_CPU_CORES; cpuId++) {
        const auto cpuTime = cpuTimes[cpuId];
        const auto previousCpuTime = mPreviousCpuTimes[cpuId];
        auto recentIdleTime = cpuTime.idleTime - previousCpuTime.idleTime;
        const auto recentTotalTime = cpuTime.totalTime - previousCpuTime.totalTime;
        if (recentIdleTime > recentTotalTime) {
            // This happens occasionally, as we use the idle time from the kernel, and the current
            // time from userspace.
            recentIdleTime = recentTotalTime;
        }
        const double idleTimePercentage =
                static_cast<double>(recentIdleTime.count()) / recentTotalTime.count();
        (*cpuCoreIdleTimesPercentage)[cpuId] = idleTimePercentage;
    }
    mPreviousCpuTimes = cpuTimes;
    return true;
}

void CpuLoadReaderSysDevices::DumpToStream(std::stringstream &stream) const {
    stream << "CPU loads from /sys/devices/system/cpu/cpuN/cpuidle:\n";
    for (size_t cpuId = 0; cpuId < NUM_CPU_CORES; cpuId++) {
        stream << "- CPU=" << cpuId << ", idleTime=" << mPreviousCpuTimes[cpuId].idleTime.count()
               << "ms, totalTime=" << mPreviousCpuTimes[cpuId].totalTime.count() << "ms\n";
    }
}

bool CpuLoadReaderSysDevices::ReadCpuTimes(std::array<CpuTime, NUM_CPU_CORES> *result) const {
    ATRACE_CALL();
    const auto totalTime = mTimeSource->GetTime();

    for (size_t cpuId = 0; cpuId < NUM_CPU_CORES; cpuId++) {
        std::chrono::microseconds idleTime;
        for (const auto &idleStateName : mIdleStateNames) {
            std::stringstream cpuIdlePath;
            cpuIdlePath << "/sys/devices/system/cpu/"
                        << "cpu" << cpuId << "/cpuidle/" << idleStateName << "/time";
            std::unique_ptr<std::istream> file;
            if (!mFilesystem->ReadFileStream(cpuIdlePath.str(), &file)) {
                return false;
            }
            // Times are reported in microseconds:
            // https://www.kernel.org/doc/Documentation/cpuidle/sysfs.txt
            std::string idleTimeUs(std::istreambuf_iterator<char>(*file), {});
            idleTime += std::chrono::microseconds(std::atoi(idleTimeUs.c_str()));
        }
        (*result)[cpuId] = {
                .idleTime = idleTime,
                .totalTime = std::chrono::duration_cast<std::chrono::microseconds>(totalTime),
        };
    }

    return true;
}

bool CpuLoadReaderSysDevices::ReadIdleStateNames(std::vector<std::string> *result) const {
    std::vector<std::string> idleStateNames;
    if (!mFilesystem->ListDirectory("/sys/devices/system/cpu/cpu0/cpuidle", &idleStateNames)) {
        return false;
    }
    for (const auto &idleStateName : idleStateNames) {
        if (idleStateName.length() == 0 || idleStateName[0] == '.') {
            continue;
        }
        std::vector<std::string> files;
        if (!mFilesystem->ListDirectory(
                    std::string("/sys/devices/system/cpu/cpu0/cpuidle/") + idleStateName, &files)) {
            return false;
        }
        if (std::find(files.begin(), files.end(), "time") == files.end()) {
            continue;
        }
        result->push_back(idleStateName);
    }
    if (idleStateNames.empty()) {
        LOG(ERROR) << "Found no idle state names";
        return false;
    }
    return true;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
