/*
 * Copyright (C) 2022 The Android Open Source Project
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

#include "adaptivecpu/KernelCpuFeatureReader.h"
#include "mocks.h"

using testing::_;
using testing::ByMove;
using testing::MatchesRegex;
using testing::Return;
using std::chrono_literals::operator""ns;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

TEST(KernelCpuFeatureReaderTest, valid) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetKernelTime())
            .Times(2)
            .WillOnce(Return(100ns))
            .WillOnce(Return(200ns));

    EXPECT_CALL(*filesystem, ReadFileStream("/proc/vendor_sched/acpu_stats", _))
            .WillOnce([](auto path __attribute__((unused)), auto result) {
                // Empty file, we will populate in ResetFileStream.
                *result = std::make_unique<std::istringstream>("");
                return true;
            });

    EXPECT_CALL(*filesystem, ResetFileStream(_))
            .Times(2)
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 100},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 100},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 100},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 100},
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 200},
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 200},
                        {.weighted_sum_freq = 300, .total_idle_time_ns = 200},
                        {.weighted_sum_freq = 300, .total_idle_time_ns = 200},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            })
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 150},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 150},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 150},
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 150},
                        {.weighted_sum_freq = 300, .total_idle_time_ns = 300},
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 300},
                        {.weighted_sum_freq = 400, .total_idle_time_ns = 300},
                        {.weighted_sum_freq = 300, .total_idle_time_ns = 300},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            });

    KernelCpuFeatureReader reader(std::move(filesystem), std::move(timeSource));
    ASSERT_TRUE(reader.Init());

    std::array<double, NUM_CPU_POLICIES> cpuPolicyAverageFrequencyHz;
    std::array<double, NUM_CPU_CORES> cpuCoreIdleTimesPercentage;
    ASSERT_TRUE(
            reader.GetRecentCpuFeatures(&cpuPolicyAverageFrequencyHz, &cpuCoreIdleTimesPercentage));
    std::array<double, NUM_CPU_POLICIES> expectedFrequencies{{1, 1, 1}};
    std::array<double, NUM_CPU_CORES> expectedIdleTimes{{0.5, 0.5, 0.5, 0.5, 1, 1, 1, 1}};
    ASSERT_EQ(cpuPolicyAverageFrequencyHz, expectedFrequencies);
    ASSERT_EQ(cpuCoreIdleTimesPercentage, expectedIdleTimes);
}

TEST(KernelCpuFeatureReaderTest, noFile) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*filesystem, ReadFileStream("/proc/vendor_sched/acpu_stats", _))
            .WillOnce(Return(false));

    KernelCpuFeatureReader reader(std::move(filesystem), std::move(timeSource));
    ASSERT_FALSE(reader.Init());
}

TEST(KernelCpuFeatureReaderTest, frequencies_capsNegativeDiff) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetKernelTime())
            .Times(2)
            .WillOnce(Return(100ns))
            .WillOnce(Return(200ns));

    EXPECT_CALL(*filesystem, ReadFileStream("/proc/vendor_sched/acpu_stats", _))
            .WillOnce([](auto path __attribute__((unused)), auto result) {
                // Empty file, we will populate in ResetFileStream.
                *result = std::make_unique<std::istringstream>("");
                return true;
            });

    EXPECT_CALL(*filesystem, ResetFileStream(_))
            .Times(2)
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 100},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            })
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 150},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            });

    KernelCpuFeatureReader reader(std::move(filesystem), std::move(timeSource));
    ASSERT_TRUE(reader.Init());

    std::array<double, NUM_CPU_POLICIES> cpuPolicyAverageFrequencyHz;
    std::array<double, NUM_CPU_CORES> cpuCoreIdleTimesPercentage;
    ASSERT_TRUE(
            reader.GetRecentCpuFeatures(&cpuPolicyAverageFrequencyHz, &cpuCoreIdleTimesPercentage));
    std::array<double, NUM_CPU_POLICIES> expectedFrequencies{{0}};
    ASSERT_EQ(cpuPolicyAverageFrequencyHz, expectedFrequencies);
}

TEST(KernelCpuFeatureReaderTest, idleTimes_capsNegativeDiff) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    std::unique_ptr<MockTimeSource> timeSource = std::make_unique<MockTimeSource>();

    EXPECT_CALL(*timeSource, GetKernelTime())
            .Times(2)
            .WillOnce(Return(100ns))
            .WillOnce(Return(200ns));

    EXPECT_CALL(*filesystem, ReadFileStream("/proc/vendor_sched/acpu_stats", _))
            .WillOnce([](auto path __attribute__((unused)), auto result) {
                // Empty file, we will populate in ResetFileStream.
                *result = std::make_unique<std::istringstream>("");
                return true;
            });

    EXPECT_CALL(*filesystem, ResetFileStream(_))
            .Times(2)
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 100, .total_idle_time_ns = 150},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            })
            .WillOnce([](auto &result) {
                std::array<acpu_stats, NUM_CPU_CORES> acpuStats{{
                        {.weighted_sum_freq = 200, .total_idle_time_ns = 100},
                }};
                char *bytes = reinterpret_cast<char *>(acpuStats.begin());
                static_cast<std::istringstream &>(*result).str(
                        std::string(bytes, bytes + sizeof(acpuStats)));
                return true;
            });

    KernelCpuFeatureReader reader(std::move(filesystem), std::move(timeSource));
    ASSERT_TRUE(reader.Init());

    std::array<double, NUM_CPU_POLICIES> cpuPolicyAverageFrequencyHz;
    std::array<double, NUM_CPU_CORES> cpuCoreIdleTimesPercentage;
    ASSERT_TRUE(
            reader.GetRecentCpuFeatures(&cpuPolicyAverageFrequencyHz, &cpuCoreIdleTimesPercentage));
    std::array<double, NUM_CPU_CORES> expectedIdleTimes{{0}};
    ASSERT_EQ(cpuCoreIdleTimesPercentage, expectedIdleTimes);
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
