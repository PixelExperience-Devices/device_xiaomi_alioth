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

#include "RealFilesystem.h"

#include <android-base/logging.h>
#include <dirent.h>
#include <utils/Trace.h>

#include <fstream>
#include <istream>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

bool RealFilesystem::ListDirectory(const std::string &path,
                                   std::vector<std::string> *result) const {
    ATRACE_CALL();
    // We can't use std::filesystem, see aosp/894015 & b/175635923.
    auto dir = std::unique_ptr<DIR, decltype(&closedir)>{opendir(path.c_str()), closedir};
    if (!dir) {
        LOG(ERROR) << "Failed to open directory " << path;
        return false;
    }
    dirent *entry;
    while ((entry = readdir(&*dir)) != nullptr) {
        result->emplace_back(entry->d_name);
    }
    return true;
}

bool RealFilesystem::ReadFileStream(const std::string &path,
                                    std::unique_ptr<std::istream> *result) const {
    ATRACE_CALL();
    *result = std::make_unique<std::ifstream>(path);
    if ((*result)->fail()) {
        LOG(ERROR) << "Failed to read file stream: " << path;
        return false;
    }
    return true;
}

bool RealFilesystem::ResetFileStream(const std::unique_ptr<std::istream> &fileStream) const {
    if (fileStream->seekg(0).bad()) {
        LOG(ERROR) << "Failed to reset file stream";
        return false;
    }
    return true;
}

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
