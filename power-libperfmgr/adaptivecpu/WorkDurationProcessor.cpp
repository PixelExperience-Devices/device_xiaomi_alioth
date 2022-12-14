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

#include "WorkDurationProcessor.h"

#include <android-base/logging.h>
#include <utils/Trace.h>

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""ns;

// The standard target duration, based on 60 FPS. Durations submitted with different targets are
// normalized against this target. For example, a duration that was at 80% of its target will be
// scaled to 0.8 * kNormalTargetDuration.
constexpr std::chrono::nanoseconds kNormalTargetDuration = 16666666ns;

// All durations shorter than this are ignored.
constexpr std::chrono::nanoseconds kMinDuration = 0ns;

// All durations longer than this are ignored.
constexpr std::chrono::nanoseconds kMaxDuration = 600 * kNormalTargetDuration;

// If we haven't processed a lot of batches, stop accepting new ones. In cases where the processing
// thread has crashed, but the reporting thread is still reporting, this prevents consuming large
// amounts of memory.
// TODO(b/213160386): Move to AdaptiveCpuConfig.
constexpr size_t kMaxUnprocessedBatches = 1000;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

bool WorkDurationProcessor::ReportWorkDurations(const std::vector<WorkDuration> &workDurations,
                                                std::chrono::nanoseconds targetDuration) {
    ATRACE_CALL();
    LOG(VERBOSE) << "Received " << workDurations.size() << " work durations with target "
                 << targetDuration.count() << "ns";
    std::unique_lock<std::mutex> lock(mMutex);
    if (mWorkDurationBatches.size() >= kMaxUnprocessedBatches) {
        LOG(ERROR) << "Adaptive CPU isn't processing work durations fast enough";
        mWorkDurationBatches.clear();
        return false;
    }
    mWorkDurationBatches.emplace_back(workDurations, targetDuration);
    return true;
}

WorkDurationFeatures WorkDurationProcessor::GetFeatures() {
    ATRACE_CALL();

    std::vector<WorkDurationBatch> workDurationBatches;
    {
        ATRACE_NAME("lock");
        std::unique_lock<std::mutex> lock(mMutex);
        mWorkDurationBatches.swap(workDurationBatches);
    }

    std::chrono::nanoseconds durationsSum = 0ns;
    std::chrono::nanoseconds maxDuration = 0ns;
    uint32_t numMissedDeadlines = 0;
    uint32_t numDurations = 0;
    for (const WorkDurationBatch &batch : workDurationBatches) {
        for (const WorkDuration workDuration : batch.workDurations) {
            std::chrono::nanoseconds duration(workDuration.durationNanos);
            if (duration < kMinDuration || duration > kMaxDuration) {
                continue;
            }

            // Normalise the duration and add it to the total.
            // kMaxDuration * kStandardTarget.count() fits comfortably within int64_t.
            std::chrono::nanoseconds durationNormalized =
                    (duration * kNormalTargetDuration.count()) / batch.targetDuration.count();
            durationsSum += durationNormalized;
            maxDuration = std::max(maxDuration, durationNormalized);
            if (duration > batch.targetDuration) {
                ++numMissedDeadlines;
            }
            ++numDurations;
        }
    }

    std::chrono::nanoseconds averageDuration = durationsSum / numDurations;
    return {
            .averageDuration = averageDuration,
            .maxDuration = maxDuration,
            .numMissedDeadlines = numMissedDeadlines,
            .numDurations = numDurations,
    };
}

bool WorkDurationProcessor::HasWorkDurations() {
    std::unique_lock<std::mutex> lock(mMutex);
    return !mWorkDurationBatches.empty();
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
