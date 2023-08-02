/**
 * @file
 * A collection of aliases to `std::multiset` that use the secure allocators.
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

#include <set>

namespace ec::unserialized_secure {
/**
 * @brief
 * Alias of `std::multiset<>` that wraps the real alloctor with
 * `ec::unserialized_secure_allocator<>`.
 *
 * @tparam Key          The key type stored in the multiset.
 * @tparam Compare      The comparison functor type.
 * @tparam Allocator    The real allocator (default: `std:allocator<Key>`).
 */
template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
using multiset = std::multiset<Key, Compare, ec::unserialized_secure_allocator<Key, Allocator>>;
}    // namespace ec::unserialized_secure

namespace ec::serialized_secure {
/**
 * @brief
 * Alias of `std::multiset<>` that wraps the real alloctor with `ec::serialized_secure_allocator<>`.
 *
 * @tparam Key          The key type stored in the multiset.
 * @tparam Compare      The comparison functor type.
 * @tparam Allocator    The real allocator (default: `std:allocator<Key>`).
 */
template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
using multiset = std::multiset<Key, Compare, ec::serialized_secure_allocator<Key, Allocator>>;
}    // namespace ec::serialized_secure
