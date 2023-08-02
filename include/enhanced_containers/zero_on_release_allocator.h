/**
 * @file
 * C++ allocator usable by STL containers that wipes memory on destruction.
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

#include <algorithm>
#include <memory>

#if __cplusplus >= 201603L
#include <memory_resource>
#endif

namespace ec {

/**
 * @brief
 * This is a C++ STL compatible allocator adapter that will ensure that the allocated memory is
 * zeroed out on deallocation.
 *
 * @code
 * if (condition) {
 *     using zero_on_release_string =  std::basic_string<char, std::char_traits<char>,
 *                                                       ec::zero_on_release_allocator<char>>;
 *     zero_on_release_string data = read_from_console();
 *     process_data(data);  // Takes a std::string_view.
 * }  // data destroyed - memory automatically zeroed out here.
 * @endcode
 *
 * Important note: Some care must be taken with certain containers.  For example, `std::string` is
 *                 allowed to use an optimization called Short String Optimization (SSO).  This
 *                 means that for strings below a certain length, the `std::string` implementation
 *                 may not actually allocate any memory.  Such strings will not be zeored out on
 *                 deallocation.  Whether or not SSO is employed and what the maximum length of a
 *                 "short string" is, is implementation defined.
 *
 * @tparam T    The type being allocated.
 * @tparam A    The actual allocator being wrapped.
 */
template <typename T, typename A = std::allocator<T>>
struct zero_on_release_allocator {
  private:
    /// @brief Type alias for the upstream allocator.
    using upstream_allocator = A;
    /// @brief Alias for allocator traits.
    using upstream_traits = std::allocator_traits<upstream_allocator>;

  public:
    /// @brief Type alias for the type being allocated.
    using value_type = typename upstream_traits::value_type;
    /// @brief Type alias for the type representing the size of allocations.
    using size_type = typename upstream_traits::size_type;
    /// @brief Type alias for the type representing the distance between pointers.
    using difference_type = typename upstream_traits::difference_type;
    /// @brief Compile-time indication about how to handle the allocator when copying containers.
    using propagate_on_container_copy_assignment =
        typename upstream_traits::propagate_on_container_copy_assignment;
    /// @brief Compile-time indication about how to handle the allocator when moving containers.
    using propagate_on_container_move_assignment =
        typename upstream_traits::propagate_on_container_move_assignment;
    /// @brief Compile-time indication about how to handle the allocator when swapping containers.
    using propagate_on_container_swap = typename upstream_traits::propagate_on_container_swap;
    /**
     * @brief Compile-time indication about how whether different instances of the allocator are
     * considered the same or not.
     */
    using is_always_equal = typename upstream_traits::is_always_equal;

    /**
     * @internal @brief
     * Define the rebind struct so that std::allocator_traits knows how to properly apply new
     * template parameter values.
     */
    template <typename U, typename... Us>
    struct rebind {
        /// @brief The rebound allocator type.
        using other =
            zero_on_release_allocator<U, typename upstream_traits::template rebind_alloc<U, Us...>>;
    };

    /// @brief Default constructor.
    zero_on_release_allocator() = default;
    /// @brief Move constructor.
    zero_on_release_allocator(zero_on_release_allocator&&) = default;
    /// @brief Copy constructor.
    zero_on_release_allocator(const zero_on_release_allocator&) = default;

    /**
     * @brief
     * Constructor to move from an alternate allocation type.
     *
     * @tparam Ts   The type parameters for the alternate form to move from.
     *
     * @param other     The allocator being moved from.
     */
    template <typename... Ts>
    zero_on_release_allocator(zero_on_release_allocator<Ts...>&& other):
        _upstream_allocator(std::move(other._upstream_allocator)) {}

    /**
     * @brief
     * Constructor to copy from an alternate allocation type.
     *
     * @tparam Ts   The type parameters for the alternate form to copy from.
     *
     * @param other     The allocator being copied from.
     */
    template <typename... Ts>
    zero_on_release_allocator(const zero_on_release_allocator<Ts...>& other):
        _upstream_allocator(other._upstream_allocator) {}

    /**
     * @brief
     * Allocate the requested amount of memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
    EC_NODISCARD

    EC_CONSTEXPR_ALLOC
    T* allocate(std::size_t len) {
        T* ptr = _upstream_allocator.allocate(len);
        return ptr;
    }

#if defined(__cpp_lib_allocate_at_least)
    /**
     * @brief
     * Allocate at least the requested amount of memory.
     *
     * Important note: This will zero out the memory so before the upstream allocator is invoked to
     * deallocate the memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
    EC_NODISCARD

    EC_CONSTEXPR_ALLOC
    auto allocate_at_least(std::size_t len) { return _upstream_allocator.allocate_at_least(len); }
#endif

    /**
     * @brief
     * Deallocate a block of memory.
     *
     * @param ptr   Address of the memory to be deallcoated.
     * @param len   Number of type T to deallocate.
     */
    EC_CONSTEXPR_ALLOC
    void deallocate(T* ptr, std::size_t len) {
        std::fill(reinterpret_cast<std::uint8_t*>(ptr), reinterpret_cast<std::uint8_t*>(ptr + len),
                  0);
        _upstream_allocator.deallocate(ptr, len);
    }

  private:
    upstream_allocator _upstream_allocator{};    ///< @brief The real allocator that will manage the
                                                 /// actual memory allocations.

    template <typename, typename>
    friend class zero_on_release_allocator;
};

}    // namespace ec
