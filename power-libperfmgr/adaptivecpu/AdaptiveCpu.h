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

#include <aidl/android/hardware/power/WorkDuration.h>
#include <perfmgr/HintManager.h>

#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>

#include "AdaptiveCpuConfig.h"
#include "AdaptiveCpuStats.h"
#include "Device.h"
#include "KernelCpuFeatureReader.h"
#include "Model.h"
#include "WorkDurationProcessor.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using std::chrono_literals::operator""ms;
using ::aidl::android::hardware::power::WorkDuration;
using ::android::perfmgr::HintManager;

// Applies CPU frequency hints infered by an ML model based on the recent CPU statistics and work
// durations.
// This class's public members are not synchronised and should not be used from multiple threads,
// with the exception of ReportWorkDuration, which can be called from an arbitrary thread.
class AdaptiveCpu {
  public:
    AdaptiveCpu();

    bool IsEnabled() const;

    // Called when the Adaptive CPU hint is received. This method enables/disables the Adaptive CPU
    // thread.
    void HintReceived(bool enable);

    // Reports work durations for processing. This method returns immediately as work durations are
    // processed asynchonuously.
    void ReportWorkDurations(const std::vector<WorkDuration> &workDurations,
                             std::chrono::nanoseconds targetDuration);

    // Dump info to a file descriptor. Called when dumping service info.
    void DumpToFd(int fd) const;

    // When PowerExt receives a hint with this name, HintReceived() is called.
    static constexpr char HINT_NAME[] = "ADAPTIVE_CPU";

  private:
    void StartThread();

    void SuspendThread();

    // The main loop of Adaptive CPU, which runs in a separate thread.
    void RunMainLoop();

    void WaitForEnabledAndWorkDurations();

    Model mModel;
    WorkDurationProcessor mWorkDurationProcessor;
    KernelCpuFeatureReader mKernelCpuFeatureReader;
    AdaptiveCpuStats mAdaptiveCpuStats;
    const TimeSource mTimeSource;

    // The thread in which work durations are processed.
    std::thread mLoopThread;

    // Guards against creating multiple threads in the case HintReceived(true) is called on separate
    // threads simultaneously.
    std::mutex mThreadCreationMutex;
    // Used when waiting in WaitForEnabledAndWorkDurations().
    std::mutex mWaitMutex;

    // A condition variable that will be notified when new work durations arrive.
    std::condition_variable mWorkDurationsAvailableCondition;

    volatile bool mIsEnabled = false;
    bool mIsInitialized = false;
    volatile bool mShouldReloadConfig = false;
    std::chrono::nanoseconds mLastEnabledHintTime;
    std::chrono::nanoseconds mLastThrottleHintTime;
    Device mDevice;
    AdaptiveCpuConfig mConfig = AdaptiveCpuConfig::DEFAULT;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
