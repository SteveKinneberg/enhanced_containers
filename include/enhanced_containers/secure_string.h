/**
 * @file
 * A collection of aliases to `std::string` that use the secure allocators.
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
#include <string>

namespace ec::unserialized_secure {
/**
 * @brief
 * Alias of `std::basic_string<>` that wraps the real alloctor with `ec::unserialized_secure_allocator<>`.
 *
 * @tparam CharT        The character type.
 * @tparam Traits       The character type traits (default: `std::char_traits<CharT>`).
 * @tparam Allocator    The real allocator (default: `std:allocator<CharT>`).
 */
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string = std::basic_string<CharT, Traits, ec::unserialized_secure_allocator<CharT, Allocator>>;

/// @brief Type alias for `char` strings using the `ec::unserialized_secure_allocator<>`.
using string = basic_string<char>;
/// @brief Type alias for `wchar_t` strings using the `ec::unserialized_secure_allocator<>`.
using wstring = basic_string<wchar_t>;
/// @brief Type alias for `char8_t` strings using the `ec::unserialized_secure_allocator<>`.
using u8string = basic_string<char8_t>;
/// @brief Type alias for `char16_t` strings using the `ec::unserialized_secure_allocator<>`.
using u16string = basic_string<char16_t>;
/// @brief Type alias for `char32_t` strings using the `ec::unserialized_secure_allocator<>`.
using u32string = basic_string<char32_t>;
}

namespace ec::serialized_secure {
/**
 * @brief
 * Alias of `std::basic_string<>` that wraps the real alloctor with `ec::serialized_secure_allocator<>`.
 *
 * @tparam CharT        The character type.
 * @tparam Traits       The character type traits (default: `std::char_traits<CharT>`).
 * @tparam Allocator    The real allocator (default: `std:allocator<CharT>`).
 */
template <typename CharT,
          typename Traits = std::char_traits<CharT>,
          typename Allocator = std::allocator<CharT>>
using basic_string = std::basic_string<CharT, Traits, ec::serialized_secure_allocator<CharT, Allocator>>;

/// @brief Type alias for `char` strings using the `ec::serialized_secure_allocator<>`.
using string = basic_string<char>;
/// @brief Type alias for `wchar_t` strings using the `ec::serialized_secure_allocator<>`.
using wstring = basic_string<wchar_t>;
/// @brief Type alias for `char8_t` strings using the `ec::serialized_secure_allocator<>`.
using u8string = basic_string<char8_t>;
/// @brief Type alias for `char16_t` strings using the `ec::serialized_secure_allocator<>`.
using u16string = basic_string<char16_t>;
/// @brief Type alias for `char32_t` strings using the `ec::serialized_secure_allocator<>`.
using u32string = basic_string<char32_t>;
}

