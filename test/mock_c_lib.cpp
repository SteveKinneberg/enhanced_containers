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

#include "mock_c_lib.h"

#include "mock_memory.h"

#include <memory>

namespace real {
int (*mlock)(const void*, std::size_t){};
int (*munlock)(const void*, std::size_t){};
void* (*memset)(void*, int, std::size_t){};
}    // namespace real

namespace {
void bind_real() {
    if (!real::mlock) {
        real::mlock =
            reinterpret_cast<int (*)(const void*, std::size_t)>(dlsym(RTLD_NEXT, "mlock"));
        real::munlock =
            reinterpret_cast<int (*)(const void*, std::size_t)>(dlsym(RTLD_NEXT, "munlock"));
        real::memset =
            reinterpret_cast<void* (*)(void*, int, std::size_t)>(dlsym(RTLD_NEXT, "memset"));
    }
}
}    // namespace

namespace mock {

std::shared_ptr<c_lib> c_lib::_self;

std::shared_ptr<c_lib> c_lib::get_instance() {
    if (!_self) { _self.reset(new c_lib{}); }
    return _self;
}

#if defined(__linux__) || defined(__unix) || defined(__unix__)
int c_lib::mock_mlock(const void* addr, std::size_t len) noexcept {
    bind_real();
    if (_self && memory::get_instance()->is_mock_memory(addr)) { return _self->mlock(addr, len); }
    return real::mlock(addr, len);
}

int c_lib::mock_munlock(const void* addr, std::size_t len) noexcept {
    bind_real();
    if (_self && memory::get_instance()->is_mock_memory(addr)) { return _self->munlock(addr, len); }
    return real::munlock(addr, len);
}

void* c_lib::mock_memset(void* s, int c, std::size_t n) noexcept {
    bind_real();
    if (_self && memory::get_instance()->is_mock_memory(s)) {
        real::memset(s, c, n);
        return _self->memset(s, c, n);
    }
    return real::memset(s, c, n);
}
#endif

}    // namespace mock


#if defined(__linux__) || defined(__unix) || defined(__unix__)

extern "C" {

int mlock(const void* addr, std::size_t len) { return mock::c_lib::mock_mlock(addr, len); }

int munlock(const void* addr, std::size_t len) { return mock::c_lib::mock_munlock(addr, len); }

void* memset(void* s, int c, std::size_t n) { return mock::c_lib::mock_memset(s, c, n); }
}
#endif
