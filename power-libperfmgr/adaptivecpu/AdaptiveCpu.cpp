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

#include "AdaptiveCpu.h"

#include <android-base/file.h>
#include <android-base/logging.h>
#include <android-base/properties.h>
#include <perfmgr/HintManager.h>
#include <sys/resource.h>
#include <utils/Trace.h>

#include <chrono>
#include <deque>
#include <numeric>

#include "CpuLoadReaderSysDevices.h"
#include "Model.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using ::android::perfmgr::HintManager;

// We pass the previous N ModelInputs to the model, including the most recent ModelInput.
constexpr uint32_t kNumHistoricalModelInputs = 3;

// TODO(b/207662659): Add config for changing between different reader types.
AdaptiveCpu::AdaptiveCpu() {}

bool AdaptiveCpu::IsEnabled() const {
    return mIsEnabled;
}

void AdaptiveCpu::HintReceived(bool enable) {
    ATRACE_CALL();
    LOG(INFO) << "AdaptiveCpu received hint: enable=" << enable;
    if (enable) {
        StartThread();
    } else {
        SuspendThread();
    }
}

void AdaptiveCpu::StartThread() {
    ATRACE_CALL();
    std::lock_guard lock(mThreadCreationMutex);
    LOG(INFO) << "Starting AdaptiveCpu thread";
    mIsEnabled = true;
    mShouldReloadConfig = true;
    mLastEnabledHintTime = mTimeSource.GetTime();
    if (!mLoopThread.joinable()) {
        mLoopThread = std::thread([&]() {
            pthread_setname_np(pthread_self(), "AdaptiveCpu");
            // Parent threads may have higher priorities, so we reset to the default.
            int ret = setpriority(PRIO_PROCESS, 0, 0);
            if (ret != 0) {
                PLOG(ERROR) << "setpriority on AdaptiveCpu thread failed: " << ret;
            }
            LOG(INFO) << "Started AdaptiveCpu thread successfully";
            RunMainLoop();
            LOG(ERROR) << "AdaptiveCpu thread ended, this should never happen!";
        });
    }
}

void AdaptiveCpu::SuspendThread() {
    ATRACE_CALL();
    LOG(INFO) << "Stopping AdaptiveCpu thread";
    // This stops the thread from receiving work durations in ReportWorkDurations, which means the
    // thread blocks indefinitely.
    mIsEnabled = false;
}

void AdaptiveCpu::ReportWorkDurations(const std::vector<WorkDuration> &workDurations,
                                      std::chrono::nanoseconds targetDuration) {
    ATRACE_CALL();
    if (!mIsEnabled) {
        return;
    }
    if (!mWorkDurationProcessor.ReportWorkDurations(workDurations, targetDuration)) {
        mIsEnabled = false;
        return;
    }
    mWorkDurationsAvailableCondition.notify_one();
}

void AdaptiveCpu::WaitForEnabledAndWorkDurations() {
    ATRACE_CALL();
    std::unique_lock<std::mutex> lock(mWaitMutex);
    // TODO(b/188770301) Once the gating logic is implemented, don't block indefinitely.
    mWorkDurationsAvailableCondition.wait(
            lock, [&] { return mIsEnabled && mWorkDurationProcessor.HasWorkDurations(); });
}

void AdaptiveCpu::RunMainLoop() {
    ATRACE_CALL();

    std::deque<ModelInput> historicalModelInputs;
    ThrottleDecision previousThrottleDecision = ThrottleDecision::NO_THROTTLE;
    while (true) {
        ATRACE_NAME("loop");
        WaitForEnabledAndWorkDurations();

        if (mLastEnabledHintTime + mConfig.enabledHintTimeout < mTimeSource.GetTime()) {
            LOG(INFO) << "Adaptive CPU hint timed out, last enabled time="
                      << mLastEnabledHintTime.count() << "ns";
            mIsEnabled = false;
            continue;
        }

        if (mShouldReloadConfig) {
            if (!AdaptiveCpuConfig::ReadFromSystemProperties(&mConfig)) {
                mIsEnabled = false;
                continue;
            }
            LOG(INFO) << "Read config: " << mConfig;
            mShouldReloadConfig = false;
        }

        ATRACE_BEGIN("compute");
        mAdaptiveCpuStats.RegisterStartRun();

        if (!mIsInitialized) {
            if (!mKernelCpuFeatureReader.Init()) {
                mIsEnabled = false;
                continue;
            }
            mDevice = ReadDevice();
            mIsInitialized = true;
        }

        ModelInput modelInput;
        modelInput.previousThrottleDecision = previousThrottleDecision;

        modelInput.workDurationFeatures = mWorkDurationProcessor.GetFeatures();
        LOG(VERBOSE) << "Got work durations: count=" << modelInput.workDurationFeatures.numDurations
                     << ", average=" << modelInput.workDurationFeatures.averageDuration.count()
                     << "ns";
        if (modelInput.workDurationFeatures.numDurations == 0) {
            continue;
        }

        if (!mKernelCpuFeatureReader.GetRecentCpuFeatures(&modelInput.cpuPolicyAverageFrequencyHz,
                                                          &modelInput.cpuCoreIdleTimesPercentage)) {
            mIsEnabled = false;
            continue;
        }

        modelInput.LogToAtrace();
        historicalModelInputs.push_back(modelInput);
        if (historicalModelInputs.size() > kNumHistoricalModelInputs) {
            historicalModelInputs.pop_front();
        }

        const ThrottleDecision throttleDecision = mModel.Run(historicalModelInputs, mConfig);
        LOG(VERBOSE) << "Model decision: " << static_cast<uint32_t>(throttleDecision);
        ATRACE_INT("AdaptiveCpu_throttleDecision", static_cast<uint32_t>(throttleDecision));

        {
            ATRACE_NAME("sendHints");
            const auto now = mTimeSource.GetTime();
            // Resend the throttle hints, even if they've not changed, if the previous send is close
            // to timing out. We define "close to" as half the hint timeout, as we can't guarantee
            // we will run again before the actual timeout.
            const bool throttleHintMayTimeout =
                    now - mLastThrottleHintTime > mConfig.hintTimeout / 2;
            if (throttleDecision != previousThrottleDecision || throttleHintMayTimeout) {
                ATRACE_NAME("sendNewHints");
                mLastThrottleHintTime = now;
                for (const auto &hintName : THROTTLE_DECISION_TO_HINT_NAMES.at(throttleDecision)) {
                    HintManager::GetInstance()->DoHint(hintName, mConfig.hintTimeout);
                }
            }
            if (throttleDecision != previousThrottleDecision) {
                ATRACE_NAME("endOldHints");
                for (const auto &hintName :
                     THROTTLE_DECISION_TO_HINT_NAMES.at(previousThrottleDecision)) {
                    HintManager::GetInstance()->EndHint(hintName);
                }
                previousThrottleDecision = throttleDecision;
            }
        }

        mAdaptiveCpuStats.RegisterSuccessfulRun(previousThrottleDecision, throttleDecision,
                                                modelInput.workDurationFeatures, mConfig);
        ATRACE_END();  // compute
        {
            ATRACE_NAME("sleep");
            std::this_thread::sleep_for(mConfig.iterationSleepDuration);
        }
    }
}

void AdaptiveCpu::DumpToFd(int fd) const {
    std::stringstream result;
    result << "========== Begin Adaptive CPU stats ==========\n";
    result << "Enabled: " << mIsEnabled << "\n";
    result << "Config: " << mConfig << "\n";
    mKernelCpuFeatureReader.DumpToStream(result);
    mAdaptiveCpuStats.DumpToStream(result);
    result << "==========  End Adaptive CPU stats  ==========\n";
    if (!::android::base::WriteStringToFd(result.str(), fd)) {
        PLOG(ERROR) << "Failed to dump state to fd";
    }
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
