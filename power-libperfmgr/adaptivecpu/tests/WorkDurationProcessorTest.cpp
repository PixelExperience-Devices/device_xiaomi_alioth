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

#include <gtest/gtest.h>

#include "adaptivecpu/WorkDurationProcessor.h"

using ::aidl::android::hardware::power::WorkDuration;
using std::chrono_literals::operator""ns;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

static const std::chrono::nanoseconds kNormalTargetDuration = 16666666ns;

TEST(WorkDurationProcessorTest, GetFeatures) {
    WorkDurationProcessor processor;
    ASSERT_TRUE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count()},
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 3}},
            kNormalTargetDuration));

    const WorkDurationFeatures expected = {.averageDuration = kNormalTargetDuration * 2,
                                           .maxDuration = kNormalTargetDuration * 3,
                                           .numMissedDeadlines = 1,
                                           .numDurations = 2};
    const WorkDurationFeatures actual = processor.GetFeatures();
    ASSERT_EQ(actual, expected);
}

TEST(WorkDurationProcessorTest, GetFeatures_multipleBatches) {
    WorkDurationProcessor processor;
    ASSERT_TRUE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count()},
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 3}},
            kNormalTargetDuration));
    ASSERT_TRUE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 6},
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 2}},
            kNormalTargetDuration));

    const WorkDurationFeatures expected = {.averageDuration = kNormalTargetDuration * 3,
                                           .maxDuration = kNormalTargetDuration * 6,
                                           .numMissedDeadlines = 3,
                                           .numDurations = 4};
    const WorkDurationFeatures actual = processor.GetFeatures();
    ASSERT_EQ(actual, expected);
}

TEST(WorkDurationProcessorTest, GetFeatures_scalesDifferentTargetDurations) {
    WorkDurationProcessor processor;
    ASSERT_TRUE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 2},
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count() * 6}},
            kNormalTargetDuration * 2));

    const WorkDurationFeatures expected = {.averageDuration = kNormalTargetDuration * 2,
                                           .maxDuration = kNormalTargetDuration * 3,
                                           .numMissedDeadlines = 1,
                                           .numDurations = 2};
    const WorkDurationFeatures actual = processor.GetFeatures();
    ASSERT_EQ(actual, expected);
}

TEST(WorkDurationProcessorTest, GetFeatures_noFrames) {
    WorkDurationProcessor processor;
    const WorkDurationFeatures expected = {
            .averageDuration = 0ns, .maxDuration = 0ns, .numMissedDeadlines = 0, .numDurations = 0};
    const WorkDurationFeatures actual = processor.GetFeatures();
    ASSERT_EQ(actual, expected);
}

TEST(WorkDurationProcessorTest, HasWorkDurations) {
    WorkDurationProcessor processor;
    ASSERT_FALSE(processor.HasWorkDurations());
    ASSERT_TRUE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count()}},
            kNormalTargetDuration * 2));
    ASSERT_TRUE(processor.HasWorkDurations());
    processor.GetFeatures();
    ASSERT_FALSE(processor.HasWorkDurations());
}

TEST(WorkDurationProcessorTest, GetFeatures_tooManyUnprocessedThenClears) {
    WorkDurationProcessor processor;
    for (int i = 0; i < 1000; i++) {
        ASSERT_TRUE(processor.ReportWorkDurations(
                std::vector<WorkDuration>{
                        {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count()}},
                kNormalTargetDuration));
    }
    ASSERT_FALSE(processor.ReportWorkDurations(
            std::vector<WorkDuration>{
                    {.timeStampNanos = 0, .durationNanos = kNormalTargetDuration.count()}},
            kNormalTargetDuration));
    ASSERT_EQ(processor.GetFeatures().numDurations, 0);
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
