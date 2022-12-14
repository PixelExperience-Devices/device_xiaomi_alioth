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
#include <ostream>
#include <set>
#include <vector>

#include "IFilesystem.h"
#include "RealFilesystem.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

struct CpuPolicyAverageFrequency {
    const uint32_t policyId;
    const uint64_t averageFrequencyHz;

    bool operator==(const CpuPolicyAverageFrequency &other) const {
        return policyId == other.policyId && averageFrequencyHz == other.averageFrequencyHz;
    }
};

class CpuFrequencyReader {
  public:
    CpuFrequencyReader() : mFilesystem(std::make_unique<RealFilesystem>()) {}
    CpuFrequencyReader(std::unique_ptr<IFilesystem> filesystem)
        : mFilesystem(std::move(filesystem)) {}

    // Initialize reading, must be done before calling other methods.
    // Work is not done in constructor as it accesses files.
    // Returns true on success.
    bool Init();

    // Gets the average frequency each CPU policy was using, since this method was last called.
    // Results are returned sorted by policyId.
    // Returns true on success.
    bool GetRecentCpuPolicyFrequencies(std::vector<CpuPolicyAverageFrequency> *result);

    // The most recently read frequencies for each CPU policy. See readCpuPolicyFrequencies for type
    // explanation. Used for dumping to bug reports.
    std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>>
    GetPreviousCpuPolicyFrequencies() const;

  private:
    // CPU policy IDs read from /sys. Initialized in #init(). Sorted ascending.
    std::vector<uint32_t> mCpuPolicyIds;
    // The CPU frequencies when #getRecentCpuPolicyFrequencies was last called (or #init if it has
    // not been called yet).
    // See readCpuPolicyFrequencies for explanation of type.
    std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>> mPreviousCpuPolicyFrequencies;
    const std::unique_ptr<IFilesystem> mFilesystem;

    // Reads, from the /sys filesystem, the CPU frequencies used by each policy.
    // - The outer map's key is the CPU policy ID.
    // - The inner map's key is the CPU frequency in Hz.
    // - The inner map's value is the time the policy has been running at that frequency, aggregated
    //   since boot.
    // Returns true on success.
    bool ReadCpuPolicyFrequencies(
            std::map<uint32_t, std::map<uint64_t, std::chrono::milliseconds>> *result);

    bool ReadCpuPolicyIds(std::vector<uint32_t> *result) const;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
