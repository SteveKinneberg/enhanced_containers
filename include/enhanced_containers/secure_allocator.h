/**
 * @file
 * C++ allocator that is usable by STL containers that blocks allocated memory from being swapped to
 * disk and wipes memory on destruction
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

#include <enhanced_containers/no_swap_allocator.h>
#include <enhanced_containers/zero_on_release_allocator.h>

namespace ec {

/**
 * @brief
 * This is a C++ STL compatible allocator adapter that will ensure that the allocated memory is
 * zeroed out on deallocation.
 *
 * It is actually a composition of the `ec::zero_on_release_allocator<>` and
 * `ec::serialized_no_swap_allocator<>` defined in such a way as to ensure that deallocated memory
 * gets zeroed out before it gets unlocked.
 *
 * This version accesses shared global state information with serialization (i.e., mutex locking).
 * As such, it is safe to use in multi-threaded applications.
 *
 * @code
 * if (condition) {
 *     std::basic_string<char, std::char_traits<char>, ec::unserialized_secure_allocator<char>> password = read_from_console();
 *     // password locked to RAM.
 *     process_password(password);  // Takes a std::string_view.
 *  }  // password destroyed - memory automatically zeroed out here and can be swapped again.
 * @endcode
 *
 * Important note: Some care must be taken with certain containers.  For example, `std::string` is
 *                 allowed to use an optimization called Short String Optimization (SSO).  This
 *                 means that for strings below a certain length, the `std::string` implementation
 *                 may not actually allocate any memory.  Such strings can still be swapped out and
 *                 will not be zeored out on deallocation.  Whether or not SSO is employed and what
 *                 the maximum length of a "short string" is, is implementation defined.
 *
 * @tparam T    The type being allocated.
 * @tparam A    The actual allocator being wrapped.
 */
template <typename T, typename A = std::allocator<T>>
using serialized_secure_allocator = zero_on_release_allocator<T, serialized_no_swap_allocator<T, A>>;

/**
 * @brief
 * This is a C++ STL compatible allocator adapter that will ensure that the allocated memory is
 * zeroed out on deallocation.
 *
 * It is actually a composition of the `ec::zero_on_release_allocator<>` and
 * `ec::unserialized_no_swap_allocator<>` defined in such a way as to ensure that deallocated memory
 * gets zeroed out before it gets unlocked.
 *
 * This version accesses shared global state information without an kind of serialization (i.e.,
 * mutex locking).  As such, it is only suitable for single threaded applications.
 *
 * @code
 * if (condition) {
 *     std::basic_string<char, std::char_traits<char>, ec::unserialized_secure_allocator<char>> password = read_from_console();
 *     // password locked to RAM.
 *     process_password(password);  // Takes a std::string_view.
 *  }  // password destroyed - memory automatically zeroed out here and can be swapped again.
 * @endcode
 *
 * Important note: Some care must be taken with certain containers.  For example, `std::string` is
 *                 allowed to use an optimization called Short String Optimization (SSO).  This
 *                 means that for strings below a certain length, the `std::string` implementation
 *                 may not actually allocate any memory.  Such strings can still be swapped out and
 *                 will not be zeored out on deallocation.  Whether or not SSO is employed and what
 *                 the maximum length of a "short string" is, is implementation defined.
 *
 * @tparam T    The type being allocated.
 * @tparam A    The actual allocator being wrapped.
 */
template <typename T, typename A = std::allocator<T>>
using unserialized_secure_allocator = zero_on_release_allocator<T, unserialized_no_swap_allocator<T, A>>;


} // namespace ec
