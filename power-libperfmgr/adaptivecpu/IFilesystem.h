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

#include <chrono>
#include <memory>
#include <ostream>
#include <vector>

namespace aidl {
namespace google {
namespace hardware {
namespace power {
namespace impl {
namespace pixel {

// Abstracted so we can mock in tests.
class IFilesystem {
  public:
    virtual ~IFilesystem() {}
    virtual bool ListDirectory(const std::string &path, std::vector<std::string> *result) const = 0;
    virtual bool ReadFileStream(const std::string &path,
                                std::unique_ptr<std::istream> *result) const = 0;
    // Resets the file stream, so that the next read will read from the beginning.
    // This function exists in IFilesystem rather than using istream::seekg directly. This is
    // so we can mock this function in tests, allowing us to return different data on reset.
    virtual bool ResetFileStream(const std::unique_ptr<std::istream> &fileStream) const = 0;
};

}  // namespace pixel
}  // namespace impl
}  // namespace power
}  // namespace hardware
}  // namespace google
}  // namespace aidl
