/**
 * @file
 * A collection of aliases to `std::unordered_multimap` that use the secure allocators.
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
#include <unordered_map>

namespace ec::unserialized_secure {
/**
 * @brief
 * Alias of `std::unordered_multimap<>` that wraps the real alloctor with `ec::unserialized_secure_allocator<>`.
 *
 * @tparam Key          The key type stored in the unordered_multimap.
 * @tparam T            The value type stored in the unordered_multimap.
 * @tparam Hash         The hash functor type.
 * @tparam KeyEqual     The equal functor type for Key.
 * @tparam Allocator    The real allocator (default: `std:allocator<Key>`).
 */
template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
using unordered_multimap = std::unordered_multimap<Key, T, Hash, KeyEqual,
                                         ec::unserialized_secure_allocator<std::pair<const Key, T>,
                                                                           Allocator>>;
}

namespace ec::serialized_secure {
/**
 * @brief
 * Alias of `std::unordered_multimap<>` that wraps the real alloctor with `ec::serialized_secure_allocator<>`.
 *
 * @tparam Key          The value type stored in the unordered_multimap.
 * @tparam T            The value type stored in the unordered_multimap.
 * @tparam Hash         The hash functor type.
 * @tparam KeyEqual     The equal functor type for Key.
 * @tparam Allocator    The real allocator (default: `std:allocator<Key>`).
 */
template <typename Key,
          typename T,
          typename Hash = std::hash<Key>,
          typename KeyEqual = std::equal_to<Key>,
          typename Allocator = std::allocator<Key>>
using unordered_multimap = std::unordered_multimap<Key, T, Hash, KeyEqual,
                                         ec::serialized_secure_allocator<std::pair<const Key, T>,
                                                                         Allocator>>;
}
