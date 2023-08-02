/**
 * @file
 * Mock STL allocator.
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

#include <enhanced_containers/details/common.h>

#include "mock_memory.h"

#include <gmock/gmock.h>
#include <memory>

namespace mock {

class allocation_monitor {
  public:
    static std::shared_ptr<allocation_monitor> get_instance();

    MOCK_METHOD(void*, void_allocate, (std::size_t), ());
    MOCK_METHOD(void, void_deallocate, (void*, std::size_t), ());

  private:
    static std::shared_ptr<allocation_monitor> _self;

    allocation_monitor() = default;
};

template <typename T>
struct allocator {
    using value_type = T;

    allocator()                 = default;
    allocator(allocator&&)      = default;
    allocator(const allocator&) = default;

    template <typename U>
    allocator(allocator<U>&& other) {}

    template <typename U>
    allocator(const allocator<U>&) {}

    virtual ~allocator() = default;

    T* allocate(std::size_t n) {
        auto ptr = reinterpret_cast<T*>(_memory->acquire(sizeof(T) * n));
        return ptr;
    }

#if defined(__cpp_lib_allocate_at_least)
    auto allocate_at_least(std::size_t n) {
        return std::allocation_result{reinterpret_cast<T*>(_memory->acquire(sizeof(T) * n)), n};
    }

#endif

    void deallocate(T* p, std::size_t n) {}

  private:
    std::shared_ptr<memory> _memory{memory::get_instance()};
};

template <typename T>
struct monitored_allocator: allocator<T> {
    using value_type = T;

    monitored_allocator() {
        auto alloc = [this](std::size_t n) {
            auto* ptr = allocator<T>::allocate(n / sizeof(T));
            return ptr;
        };
        auto dealloc = [this](void* p, std::size_t n) {
            allocator<T>::deallocate(reinterpret_cast<T*>(p), n / sizeof(T));
        };
        ON_CALL(*_monitor, void_allocate).WillByDefault(alloc);
        ON_CALL(*_monitor, void_deallocate).WillByDefault(dealloc);
    }

    monitored_allocator(monitored_allocator&&): monitored_allocator{} {}

    monitored_allocator(const monitored_allocator&): monitored_allocator{} {}

    template <typename U>
    monitored_allocator(monitored_allocator<U>&& other): monitored_allocator{} {}

    template <typename U>
    monitored_allocator(const monitored_allocator<U>&): monitored_allocator{} {}

    T* allocate(std::size_t n) {
        auto ptr = reinterpret_cast<T*>(_monitor->void_allocate(n * sizeof(T)));
        return ptr;
    }

#if defined(__cpp_lib_allocate_at_least)
    auto allocate_at_least(std::size_t n) {
        return reinterpret_cast<T*>(_monitor->void_allocate(n * sizeof(T)));
    }

#endif
    void deallocate(T* p, std::size_t n) { _monitor->void_deallocate(p, n * sizeof(T)); }

  private:
    std::shared_ptr<allocation_monitor> _monitor{allocation_monitor::get_instance()};
};

}    // namespace mock
