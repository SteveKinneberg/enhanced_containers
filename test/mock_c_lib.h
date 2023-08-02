/**
 * @file
 * Mocks for standard C library calls.
 *
 * @copyright 2023 Steve Kinneberg <steve.kinneberg@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <gmock/gmock.h>
#include <memory>

#if defined(__linux__) || defined(__unix) || defined(__unix__)
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>

#elif defined(_WIN32)
// QUESTION: Will the technique used to mock the Linux system library functions work for Windows
// programs due to the weird way Windows programs are pseudo-linked at compile-time to their DLLs?

// Add include for APIs to access DLLs.
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <sysinfoapi.h>

#elif defined(__APPLE__) && defined(__MACOS__)
#error Not supported yet.
#endif


#if defined(__linux__) || defined(__unix) || defined(__unix__)
namespace real {
extern int (*mlock)(const void*, std::size_t);
extern int (*munlock)(const void*, std::size_t);
extern void* (*memset)(void*, int, std::size_t);
}    // namespace real
#endif

namespace mock {

struct c_lib {
    static std::shared_ptr<c_lib> get_instance();

#if defined(__linux__) || defined(__unix) || defined(__unix__)
    static int   mock_mlock(const void* addr, std::size_t len) noexcept;
    static int   mock_munlock(const void* addr, std::size_t len) noexcept;
    static void* mock_memset(void* s, int c, std::size_t n) noexcept;

    MOCK_METHOD(int, mlock, (const void* addr, std::size_t len), (noexcept));
    MOCK_METHOD(int, munlock, (const void* addr, std::size_t len), (noexcept));
    MOCK_METHOD(void*, memset, (void* s, int c, std::size_t n), (noexcept));

#elif defined(_WIN32)
#elif defined(__APPLE__) && defined(__MACOS__)
#error Not supported yet.
#endif

  private:
    static std::shared_ptr<c_lib> _self;

    c_lib() = default;
};

}    // namespace mock
