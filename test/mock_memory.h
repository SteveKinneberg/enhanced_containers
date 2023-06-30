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

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace mock {

/**
 * @brief
 * This is a simplistic abstraction for allocating memory.
 *
 * It exists to serve 2 purposes in unit tests:
 *
 *   - Provide a known memory range so that it is possible to write mock functions that operate on
 *     memory address to know when to delegate to the real version of the function and when to call
 *     the associated Google Mock function.
 *   - Enable unit tests to evaluate the contents of memory after it has been "released".
 *
 * Important note.  It does not really handle deallocation.  Unit tests are expected to call the
 * `reset()` method between each test to ensure that each test starts with memory in a known state.
 */
class memory {
  public:
    static constexpr std::size_t memory_size{1 << 16};

    static std::shared_ptr<memory> get_instance();
    void* acquire(std::size_t n);
    void reset();
    void set_next_allocation_offset(std::size_t offset);
    void fill(std::byte v = static_cast<std::byte>(0x5a));
    void fill(std::uint8_t v) { fill(static_cast<std::byte>(v)); }

    auto& get_memory_array() { return _memory; }
    const auto& get_memory_array() const { return _memory; }

    bool is_mock_memory(const void* ptr) const;


  private:
    static std::shared_ptr<memory> _self;

    alignas(memory_size)
    std::array<std::byte, memory_size> _memory;
    std::size_t _next_allocation_offset{};

    memory() = default;
};


} // namespace mock
