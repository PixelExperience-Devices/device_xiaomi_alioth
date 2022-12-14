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

#pragma once

#include <ostream>

#include "AdaptiveCpuConfig.h"
#include "ITimeSource.h"
#include "Model.h"
#include "TimeSource.h"
#include "WorkDurationProcessor.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

// Collects statistics about Adaptive CPU.
// These are only Used during a dumpsys to improve bug report quality.
class AdaptiveCpuStats {
  public:
    AdaptiveCpuStats() : mTimeSource(std::make_unique<TimeSource>()) {}
    AdaptiveCpuStats(std::unique_ptr<ITimeSource> timeSource)
        : mTimeSource(std::move(timeSource)) {}

    void RegisterStartRun();
    void RegisterSuccessfulRun(ThrottleDecision previousThrottleDecision,
                               ThrottleDecision throttleDecision,
                               WorkDurationFeatures workDurationFeatures,
                               const AdaptiveCpuConfig &config);
    void DumpToStream(std::ostream &stream) const;

  private:
    const std::unique_ptr<ITimeSource> mTimeSource;

    size_t mNumStartedRuns = 0;
    size_t mNumSuccessfulRuns = 0;
    std::chrono::nanoseconds mStartTime;
    std::chrono::nanoseconds mLastRunStartTime;
    std::chrono::nanoseconds mLastRunSuccessTime;
    std::chrono::nanoseconds mTotalRunDuration;

    std::map<ThrottleDecision, size_t> mNumThrottles;
    std::map<ThrottleDecision, std::chrono::nanoseconds> mThrottleDurations;

    std::map<ThrottleDecision, size_t> mNumDurations;
    std::map<ThrottleDecision, size_t> mNumMissedDeadlines;

    static std::string FormatDuration(std::chrono::nanoseconds duration);
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
