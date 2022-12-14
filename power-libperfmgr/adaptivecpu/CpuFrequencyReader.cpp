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

#include "CpuFrequencyReader.h"

#include <android-base/logging.h>
#include <inttypes.h>
#include <utils/Trace.h>

#include <fstream>
#include <memory>
#include <sstream>

using std::chrono_literals::operator""ms;

constexpr std::string_view kCpuPolicyDirectory("/sys/devices/system/cpu/cpufreq");

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

bool CpuFrequencyReader::Init() {
    ATRACE_CALL();
    mCpuPolicyIds.clear();
    if (!ReadCpuPolicyIds(&mCpuPolicyIds)) {
        return false;
    }
    mPreviousCpuPolicyFrequencies.clear();
    return ReadCpuPolicyFrequencies(&mPreviousCpuPolicyFrequencies);
}

bool CpuFrequencyReader::GetRecentCpuPolicyFrequencies(
        std::vector<CpuPolicyAverageFrequency> *result) {
    ATRACE_CALL();
    std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>> cpuPolicyFrequencies;
    if (!ReadCpuPolicyFrequencies(&cpuPolicyFrequencies)) {
        return false;
    }
    for (const auto &[policyId, cpuFrequencies] : cpuPolicyFrequencies) {
        const auto &previousCpuFrequencies = mPreviousCpuPolicyFrequencies.find(policyId);
        if (previousCpuFrequencies == mPreviousCpuPolicyFrequencies.end()) {
            LOG(ERROR) << "Couldn't find policy " << policyId << " in previous frequencies";
            return false;
        }
        uint64_t weightedFrequenciesSumHz = 0;
        std::chrono::milliseconds timeSum = 0ms;
        for (const auto &[frequencyHz, time] : cpuFrequencies) {
            const auto &previousCpuFrequency = previousCpuFrequencies->second.find(frequencyHz);
            if (previousCpuFrequency == previousCpuFrequencies->second.end()) {
                LOG(ERROR) << "Couldn't find frequency " << frequencyHz
                           << " in previous frequencies";
                return false;
            }
            const std::chrono::milliseconds recentTime = time - previousCpuFrequency->second;
            weightedFrequenciesSumHz += frequencyHz * recentTime.count();
            timeSum += recentTime;
        }
        const uint64_t averageFrequencyHz =
                timeSum != 0ms ? weightedFrequenciesSumHz / timeSum.count() : 0;
        result->push_back({.policyId = policyId, .averageFrequencyHz = averageFrequencyHz});
    }
    mPreviousCpuPolicyFrequencies = cpuPolicyFrequencies;
    return true;
}

std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>>
CpuFrequencyReader::GetPreviousCpuPolicyFrequencies() const {
    return mPreviousCpuPolicyFrequencies;
}

bool CpuFrequencyReader::ReadCpuPolicyFrequencies(
        std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>> *result) {
    ATRACE_CALL();
    for (const uint32_t cpuPolicyId : mCpuPolicyIds) {
        std::stringstream timeInStatePath;
        timeInStatePath << "/sys/devices/system/cpu/cpufreq/policy" << cpuPolicyId
                        << "/stats/time_in_state";
        std::unique_ptr<std::istream> timeInStateFile;
        if (!mFilesystem->ReadFileStream(timeInStatePath.str(), &timeInStateFile)) {
            return false;
        }

        std::map<uint64_t, std::chrono::milliseconds> cpuFrequencies;
        std::string timeInStateLine;
        while (std::getline(*timeInStateFile, timeInStateLine)) {
            // Time format in time_in_state is 10s of milliseconds:
            // https://www.kernel.org/doc/Documentation/cpu-freq/cpufreq-stats.txt
            uint64_t frequencyHz, time10Ms;
            if (std::sscanf(timeInStateLine.c_str(), "%" PRIu64 " %" PRIu64 "\n", &frequencyHz,
                            &time10Ms) != 2) {
                LOG(ERROR) << "Failed to parse time_in_state line: " << timeInStateLine;
                return false;
            }
            cpuFrequencies[frequencyHz] = time10Ms * 10ms;
        }
        if (cpuFrequencies.size() > 500) {
            LOG(ERROR) << "Found " << cpuFrequencies.size() << " frequencies for policy "
                       << cpuPolicyId << ", aborting";
            return false;
        }
        (*result)[cpuPolicyId] = cpuFrequencies;
    }
    return true;
}

bool CpuFrequencyReader::ReadCpuPolicyIds(std::vector<uint32_t> *result) const {
    ATRACE_CALL();
    std::vector<std::string> entries;
    if (!mFilesystem->ListDirectory(kCpuPolicyDirectory.data(), &entries)) {
        return false;
    }
    for (const auto &entry : entries) {
        uint32_t cpuPolicyId;
        if (!sscanf(entry.c_str(), "policy%d", &cpuPolicyId)) {
            continue;
        }
        result->push_back(cpuPolicyId);
    }
    // Sort the list, so that getRecentCpuPolicyFrequencies always returns frequencies sorted by
    // policy ID.
    std::sort(result->begin(), result->end());
    return true;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
