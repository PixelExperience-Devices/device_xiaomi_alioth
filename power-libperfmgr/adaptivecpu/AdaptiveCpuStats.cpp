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

#include "AdaptiveCpuStats.h"

#include <utils/Trace.h>

#include "AdaptiveCpu.h"

using std::chrono_literals::operator""ns;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

void AdaptiveCpuStats::RegisterStartRun() {
    ATRACE_CALL();
    mNumStartedRuns++;
    mLastRunStartTime = mTimeSource->GetTime();
    if (mStartTime == 0ns) {
        mStartTime = mLastRunStartTime;
    }
}

void AdaptiveCpuStats::RegisterSuccessfulRun(ThrottleDecision previousThrottleDecision,
                                             ThrottleDecision throttleDecision,
                                             WorkDurationFeatures workDurationFeatures,
                                             const AdaptiveCpuConfig &config) {
    ATRACE_CALL();
    mNumSuccessfulRuns++;
    mNumThrottles[throttleDecision]++;
    const auto runSuccessTime = mTimeSource->GetTime();
    mTotalRunDuration += runSuccessTime - mLastRunStartTime;
    // Don't update previousThrottleDecision entries if we haven't run successfully before.
    if (mLastRunSuccessTime != 0ns) {
        mThrottleDurations[previousThrottleDecision] +=
                std::min(runSuccessTime - mLastRunSuccessTime,
                         std::chrono::duration_cast<std::chrono::nanoseconds>(config.hintTimeout));
        mNumDurations[previousThrottleDecision] += workDurationFeatures.numDurations;
        mNumMissedDeadlines[previousThrottleDecision] += workDurationFeatures.numMissedDeadlines;
    }
    mLastRunSuccessTime = runSuccessTime;
}

void AdaptiveCpuStats::DumpToStream(std::ostream &stream) const {
    stream << "Stats:\n";
    stream << "- Successful runs / total runs: " << mNumSuccessfulRuns << " / " << mNumStartedRuns
           << "\n";
    stream << "- Total run duration: " << FormatDuration(mTotalRunDuration) << "\n";
    stream << "- Average run duration: " << FormatDuration(mTotalRunDuration / mNumSuccessfulRuns)
           << "\n";
    stream << "- Running time fraction: "
           << static_cast<double>(mTotalRunDuration.count()) /
                      (mTimeSource->GetTime() - mStartTime).count()
           << "\n";

    stream << "- Number of throttles:\n";
    size_t totalNumThrottles = 0;
    for (const auto &[throttleDecision, numThrottles] : mNumThrottles) {
        stream << "  - " << ThrottleString(throttleDecision) << ": " << numThrottles << "\n";
        totalNumThrottles += numThrottles;
    }
    stream << "  - Total: " << totalNumThrottles << "\n";

    stream << "- Time spent throttling:\n";
    std::chrono::nanoseconds totalThrottleDuration;
    for (const auto &[throttleDecision, throttleDuration] : mThrottleDurations) {
        stream << "  - " << ThrottleString(throttleDecision) << ": "
               << FormatDuration(throttleDuration) << "\n";
        totalThrottleDuration += throttleDuration;
    }
    stream << "  - Total: " << FormatDuration(totalThrottleDuration) << "\n";

    stream << "- Missed deadlines per throttle:\n";
    size_t totalNumDurations = 0;
    size_t totalNumMissedDeadlines = 0;
    for (const auto &[throttleDecision, numDurations] : mNumDurations) {
        const size_t numMissedDeadlines = mNumMissedDeadlines.at(throttleDecision);
        stream << "  - " << ThrottleString(throttleDecision) << ": " << numMissedDeadlines << " / "
               << numDurations << " (" << static_cast<double>(numMissedDeadlines) / numDurations
               << ")\n";
        totalNumDurations += numDurations;
        totalNumMissedDeadlines += numMissedDeadlines;
    }
    stream << "  - Total: " << totalNumMissedDeadlines << " / " << totalNumDurations << " ("
           << static_cast<double>(totalNumMissedDeadlines) / totalNumDurations << ")\n";
}

std::string AdaptiveCpuStats::FormatDuration(std::chrono::nanoseconds duration) {
    double count = static_cast<double>(duration.count());
    std::string suffix;
    if (count < 1000.0) {
        suffix = "ns";
    } else if (count < 1000.0 * 1000) {
        suffix = "us";
        count /= 1000;
    } else if (count < 1000.0 * 1000 * 100) {
        suffix = "ms";
        count /= 1000 * 1000;
    } else {
        suffix = "s";
        count /= 1000 * 1000 * 1000;
    }
    return std::to_string(count) + suffix;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
