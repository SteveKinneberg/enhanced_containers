/**
 * @file
 * Mock memory space used by other mocks that operate on memory addresses.
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

#include "mock_memory.h"

#include <algorithm>
#include <new>

namespace mock {

std::shared_ptr<memory> memory::_self;

std::shared_ptr<memory> memory::get_instance() {
    if (!_self) { _self.reset(new memory{}); }
    return _self;
}

void* memory::acquire(std::size_t n) {
    if (_next_allocation_offset + n > _memory.size()) { throw std::bad_alloc{}; }
    auto* r = &_memory[_next_allocation_offset];
    _next_allocation_offset += n;
    return r;
}

void memory::reset() { _next_allocation_offset = 0; }

void memory::set_next_allocation_offset(std::size_t offset) { _next_allocation_offset = offset; }

void memory::fill(std::byte v) { std::fill(_memory.begin(), _memory.end(), v); }

bool memory::is_mock_memory(const void* ptr) const {
    auto* bptr = reinterpret_cast<const std::byte*>(ptr);
    return (bptr >= _memory.data()) && ((bptr - _memory.data()) < _memory.size());
}

}    // namespace mock
