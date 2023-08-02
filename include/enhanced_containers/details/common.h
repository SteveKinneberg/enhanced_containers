/**
 * @internal @file
 * A collection of common items to help make the code more readable.
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

#define EC_CPP11 201103L
#define EC_CPP14 201402L
#define EC_CPP17 201703L
#define EC_CPP20 202002L
#define EC_CPP23 202302L

#if __cplusplus >= EC_CPP20
#include <version>
#endif


#if defined(__cpp_constexpr_dynamic_alloc)
#define EC_CONSTEXPR_ALLOC constexpr
#else
#define EC_CONSTEXPR_ALLOC inline
#endif

#if __has_cpp_attribute(nodiscard)
#define EC_NODISCARD [[nodiscard]]
#else
#define EC_NODISCARD
#endif
