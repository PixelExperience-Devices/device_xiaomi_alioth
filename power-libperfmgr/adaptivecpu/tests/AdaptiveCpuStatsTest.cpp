
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

#include "adaptivecpu/AdaptiveCpuStats.h"
#include "mocks.h"

using testing::HasSubstr;
using testing::Return;
using std::chrono_literals::operator""ns;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

TEST(AdaptiveCpuStatsTest, singleRun) {
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetTime())
            .Times(3)
            .WillOnce(Return(1000ns))
            .WillOnce(Return(1100ns))
            .WillOnce(Return(1200ns));

    AdaptiveCpuStats stats(std::move(timeSource));
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::NO_THROTTLE, ThrottleDecision::THROTTLE_60, {},
                                AdaptiveCpuConfig::DEFAULT);

    std::stringstream stream;
    stats.DumpToStream(stream);
    EXPECT_THAT(stream.str(), HasSubstr("- Successful runs / total runs: 1 / 1\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Total run duration: 100.000000ns\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Average run duration: 100.000000ns\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Running time fraction: 0.5\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- THROTTLE_60: 1\n"));
}

TEST(AdaptiveCpuStatsTest, multipleRuns) {
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetTime())
            .Times(9)
            .WillOnce(Return(1000ns))   // start #1
            .WillOnce(Return(1100ns))   // success #1
            .WillOnce(Return(2000ns))   // start #2
            .WillOnce(Return(2200ns))   // success #2
            .WillOnce(Return(3000ns))   // start #3
            .WillOnce(Return(3100ns))   // success #3
            .WillOnce(Return(4000ns))   // start #4
            .WillOnce(Return(4800ns))   // success #4
            .WillOnce(Return(5000ns));  // dump

    AdaptiveCpuStats stats(std::move(timeSource));
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::NO_THROTTLE, ThrottleDecision::THROTTLE_60,
                                // Ignored, as this is the first call.
                                {.numDurations = 100000, .numMissedDeadlines = 123},
                                AdaptiveCpuConfig::DEFAULT);
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::THROTTLE_60, ThrottleDecision::THROTTLE_70,
                                {.numDurations = 100, .numMissedDeadlines = 10},
                                AdaptiveCpuConfig::DEFAULT);
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::THROTTLE_70, ThrottleDecision::THROTTLE_60,
                                {.numDurations = 50, .numMissedDeadlines = 1},
                                AdaptiveCpuConfig::DEFAULT);
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::THROTTLE_60, ThrottleDecision::THROTTLE_80,
                                {.numDurations = 200, .numMissedDeadlines = 20},
                                AdaptiveCpuConfig::DEFAULT);

    std::stringstream stream;
    stats.DumpToStream(stream);
    EXPECT_THAT(stream.str(), HasSubstr("- Successful runs / total runs: 4 / 4\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Total run duration: 1.200000us\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Average run duration: 300.000000ns\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- Running time fraction: 0.3\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- THROTTLE_60: 2\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- THROTTLE_70: 1\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- THROTTLE_60: 30 / 300 (0.1)\n"));
    EXPECT_THAT(stream.str(), HasSubstr("- THROTTLE_70: 1 / 50 (0.02)\n"));
}

TEST(AdaptiveCpuStatsTest, failedRun) {
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetTime())
            .Times(4)
            .WillOnce(Return(1000ns))
            .WillOnce(Return(1100ns))
            .WillOnce(Return(1200ns))
            .WillOnce(Return(1300ns));

    AdaptiveCpuStats stats(std::move(timeSource));
    stats.RegisterStartRun();
    stats.RegisterStartRun();
    stats.RegisterSuccessfulRun(ThrottleDecision::NO_THROTTLE, ThrottleDecision::THROTTLE_60, {},
                                AdaptiveCpuConfig::DEFAULT);

    std::stringstream stream;
    stats.DumpToStream(stream);
    EXPECT_THAT(stream.str(), HasSubstr("- Successful runs / total runs: 1 / 2\n"));
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
