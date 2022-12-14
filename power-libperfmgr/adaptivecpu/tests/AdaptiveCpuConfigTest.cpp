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

#include <android-base/properties.h>
#include <gtest/gtest.h>

#include "adaptivecpu/AdaptiveCpuConfig.h"

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

using std::chrono_literals::operator""ms;
using std::chrono_literals::operator""min;

class AdaptiveCpuConfigTest : public ::testing::Test {
  protected:
    void SetUp() override {
        android::base::SetProperty("debug.adaptivecpu.iteration_sleep_duration_ms", "");
        android::base::SetProperty("debug.adaptivecpu.hint_timeout_ms", "");
        android::base::SetProperty("debug.adaptivecpu.random_throttle_decision_percent", "");
        android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "");
        android::base::SetProperty("debug.adaptivecpu.enabled_hint_timeout_ms", "");
    }
};

TEST(AdaptiveCpuConfigTest, valid) {
    android::base::SetProperty("debug.adaptivecpu.iteration_sleep_duration_ms", "25");
    android::base::SetProperty("debug.adaptivecpu.hint_timeout_ms", "500");
    android::base::SetProperty("debug.adaptivecpu.random_throttle_decision_percent", "25");
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,3,4");
    android::base::SetProperty("debug.adaptivecpu.enabled_hint_timeout_ms", "1000");
    const AdaptiveCpuConfig expectedConfig{
            .iterationSleepDuration = 25ms,
            .hintTimeout = 500ms,
            .randomThrottleDecisionProbability = 0.25,
            .enabledHintTimeout = 1000ms,
            .randomThrottleOptions = {ThrottleDecision::NO_THROTTLE, ThrottleDecision::THROTTLE_70,
                                      ThrottleDecision::THROTTLE_80},
    };
    AdaptiveCpuConfig actualConfig;
    ASSERT_TRUE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    ASSERT_EQ(actualConfig, expectedConfig);
}

TEST(AdaptiveCpuConfigTest, defaultConfig) {
    android::base::SetProperty("debug.adaptivecpu.iteration_sleep_duration_ms", "");
    android::base::SetProperty("debug.adaptivecpu.hint_timeout_ms", "");
    android::base::SetProperty("debug.adaptivecpu.random_throttle_decision_percent", "");
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "");
    android::base::SetProperty("debug.adaptivecpu.enabled_hint_timeout_ms", "");
    AdaptiveCpuConfig actualConfig;
    ASSERT_TRUE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    ASSERT_EQ(actualConfig, AdaptiveCpuConfig::DEFAULT);
}

TEST(AdaptiveCpuConfigTest, iterationSleepDuration_belowMin) {
    android::base::SetProperty("debug.adaptivecpu.iteration_sleep_duration_ms", "2");
    AdaptiveCpuConfig actualConfig;
    ASSERT_TRUE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    ASSERT_EQ(actualConfig.iterationSleepDuration, 20ms);
}

TEST(AdaptiveCpuConfigTest, iterationSleepDuration_negative) {
    android::base::SetProperty("debug.adaptivecpu.iteration_sleep_duration_ms", "-100");
    AdaptiveCpuConfig actualConfig;
    ASSERT_TRUE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    ASSERT_EQ(actualConfig.iterationSleepDuration, 1000ms);
}

TEST(AdaptiveCpuConfigTest, randomThrottleDecisionProbability_float) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_decision_percent", "0.5");
    AdaptiveCpuConfig actualConfig;
    ASSERT_TRUE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    ASSERT_EQ(actualConfig.randomThrottleDecisionProbability, 0);
}

TEST(AdaptiveCpuConfigTest, randomThrottleDecisionProbability_tooBig) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_decision_percent", "101");
    AdaptiveCpuConfig actualConfig;
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
}

TEST(AdaptiveCpuConfigTest, randomThrottleOptions_invalidThrottle) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,1,2,9");
    AdaptiveCpuConfig actualConfig;
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
}

TEST(AdaptiveCpuConfigTest, randomThrottleOptions_wrongDelim) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,1.2,3");
    AdaptiveCpuConfig actualConfig;
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
}

TEST(AdaptiveCpuConfigTest, randomThrottleOptions_wrongNumber) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,1,2a,3");
    AdaptiveCpuConfig actualConfig;
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
}

TEST(AdaptiveCpuConfigTest, randomThrottleOptions_straySpace) {
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,1 ,2,3");
    AdaptiveCpuConfig actualConfig;
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
    android::base::SetProperty("debug.adaptivecpu.random_throttle_options", "0,1,2,3 ");
    ASSERT_FALSE(AdaptiveCpuConfig::ReadFromSystemProperties(&actualConfig));
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
