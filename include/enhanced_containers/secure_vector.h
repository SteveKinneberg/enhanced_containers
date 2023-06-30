/**
 * @file
 * A collection of aliases to `std::vector` that use the secure allocators.
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

#include <enhanced_containers/secure_allocator.h>
#include <vector>

namespace ec::unserialized_secure {
/**
 * @brief
 * Alias of `std::vector<>` that wraps the real alloctor with `ec::unserialized_secure_allocator<>`.
 *
 * @tparam T            The value type stored in the vector.
 * @tparam Allocator    The real allocator (default: `std:allocator<T>`).
 */
template <typename T, typename Allocator = std::allocator<T>>
using vector = std::vector<T, ec::unserialized_secure_allocator<T, Allocator>>;
}

namespace ec::serialized_secure {
/**
 * @brief
 * Alias of `std::vector<>` that wraps the real alloctor with `ec::unserialized_secure_allocator<>`.
 *
 * @tparam T            The value type stored in the vector.
 * @tparam Allocator    The real allocator (default: `std:allocator<T>`).
 */
template <typename T, typename Allocator = std::allocator<T>>
using vector = std::vector<T, ec::serialized_secure_allocator<T, Allocator>>;
}
