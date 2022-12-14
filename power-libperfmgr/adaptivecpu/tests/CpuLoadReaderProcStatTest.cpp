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

#include "adaptivecpu/CpuLoadReaderProcStat.h"
#include "mocks.h"

#define LOG_TAG "powerhal-adaptivecpu"

using testing::_;
using testing::ByMove;
using testing::Return;

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

TEST(CpuLoadReaderProcStatTest, GetRecentCpuLoads) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    EXPECT_CALL(*filesystem, ReadFileStream("/proc/stat", _))
            .Times(2)
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 100 0 0 50 0 0 0 0 0 0\n";
                ss << "cpu2 200 0 0 50 0 0 0 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            })
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 200 0 0 150 0 0 0 0 0 0\n";
                ss << "cpu2 500 0 0 150 0 0 0 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            });

    CpuLoadReaderProcStat reader(std::move(filesystem));
    reader.Init();

    std::array<double, NUM_CPU_CORES> actualPercentages;
    ASSERT_TRUE(reader.GetRecentCpuLoads(&actualPercentages));
    std::array<double, NUM_CPU_CORES> expectedPercentages({0, 0.5, 0.25, 0, 0, 0, 0, 0});
    ASSERT_EQ(actualPercentages, expectedPercentages);
}

TEST(CpuLoadReaderProcStatTest, GetRecentCpuLoads_failsWithMissingValues) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    EXPECT_CALL(*filesystem, ReadFileStream("/proc/stat", _))
            .Times(2)
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 100 0 0 50 0 0 0\n";
                ss << "cpu2 200 0 0 50 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            })
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 200 0 0 150 0 0 0\n";
                ss << "cpu2 500 0 0 150 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            });

    CpuLoadReaderProcStat reader(std::move(filesystem));
    reader.Init();
    std::array<double, NUM_CPU_CORES> actualPercentages;
    ASSERT_FALSE(reader.GetRecentCpuLoads(&actualPercentages));
}

TEST(CpuLoadReaderProcStatTest, GetRecentCpuLoads_failsWithEmptyFile) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    EXPECT_CALL(*filesystem, ReadFileStream("/proc/stat", _))
            .Times(2)
            .WillRepeatedly([](auto _path __attribute__((unused)), auto result) {
                *result = std::make_unique<std::istringstream>("");
                return true;
            });

    CpuLoadReaderProcStat reader(std::move(filesystem));
    reader.Init();
    std::array<double, NUM_CPU_CORES> actualPercentages;
    ASSERT_FALSE(reader.GetRecentCpuLoads(&actualPercentages));
}

TEST(CpuLoadReaderProcStatTest, GetRecentCpuLoads_failsWithDifferentCpus) {
    std::unique_ptr<MockFilesystem> filesystem = std::make_unique<MockFilesystem>();
    EXPECT_CALL(*filesystem, ReadFileStream("/proc/stat", _))
            .Times(2)
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 100 0 0 50 0 0 0 0 0 0\n";
                ss << "cpu2 200 0 0 50 0 0 0 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            })
            .WillOnce([](auto _path __attribute__((unused)), auto result) {
                std::stringstream ss;
                ss << "bad line\n";
                ss << "cpu1 200 0 0 150 0 0 0 0 0 0\n";
                ss << "cpu3 500 0 0 150 0 0 0 0 0 0\n";
                *result = std::make_unique<std::istringstream>(ss.str());
                return true;
            });

    CpuLoadReaderProcStat reader(std::move(filesystem));
    reader.Init();
    std::array<double, NUM_CPU_CORES> actualPercentages;
    ASSERT_FALSE(reader.GetRecentCpuLoads(&actualPercentages));
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
