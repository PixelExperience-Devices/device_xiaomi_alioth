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

#include <aidl/android/hardware/power/WorkDuration.h>

#include <chrono>
#include <vector>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using ::aidl::android::hardware::power::WorkDuration;

struct WorkDurationBatch {
    WorkDurationBatch(const std::vector<WorkDuration> &workDurations,
                      std::chrono::nanoseconds targetDuration)
        : workDurations(workDurations), targetDuration(targetDuration) {}
    const std::vector<WorkDuration> workDurations;
    const std::chrono::nanoseconds targetDuration;
};

struct WorkDurationFeatures {
    std::chrono::nanoseconds averageDuration;
    std::chrono::nanoseconds maxDuration;
    uint32_t numMissedDeadlines;
    uint32_t numDurations;

    bool operator==(const WorkDurationFeatures &other) const {
        return averageDuration == other.averageDuration && maxDuration == other.maxDuration &&
               numMissedDeadlines == other.numMissedDeadlines && numDurations == other.numDurations;
    }
};

class WorkDurationProcessor {
  public:
    bool ReportWorkDurations(const std::vector<WorkDuration> &workDurations,
                             std::chrono::nanoseconds targetDuration);

    WorkDurationFeatures GetFeatures();

    // True if ReportWorkDurations has been called since GetFeatures was last called.
    bool HasWorkDurations();

  private:
    // The work durations reported since GetFeatures() was last called.
    // Ordered from least recent to most recent.
    std::vector<WorkDurationBatch> mWorkDurationBatches;
    // Guards reading/writing mWorkDurationBatches.
    std::mutex mMutex;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
