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

#include <enhanced_containers/details/common.h>
#include <memory>
#include <mutex>
#include <unordered_map>

#if __cplusplus >= 201603L
#include <memory_resource>
#endif

namespace ec {
namespace details {
/**
 * @internal @brief
 * Tracks reference counts to memory pages allocated via one of the no swap allocators.
 *
 * This is implemented as a signleton so that we can ensure that it gets initialized in time for use
 * by global (static) scope containers.  Each instance of the no swap allocators will get a
 * std::shared_ptr<> to ensure that this state information remains vaild for the lifetime of all
 * containers that use any of the no swap allocators.
 */
class no_swap_allocator_state {
  public:
    /**
     * @brief
     * Get a shared pointer to the state singleton object.
     *
     * @return  A shared pointer to the state singleton object.
     */
    static std::shared_ptr<no_swap_allocator_state> get_state_object();

    /**
     * @brief
     * Record a new memory allocation.
     *
     * This will lock the referenced pages of memory so that the OS will not swap them out if the
     * pages have not already be marked as such.
     *
     * @param ptr   Pointer to the newly allocated memory.
     * @param len   Number of bytes in the newly allocated memory.
     */
    void add_allocation(void* ptr, std::size_t len);

    /**
     * @brief
     * Process the deallocation of a memory region.
     *
     * This will unlock the referenced pages of memory so that the OS can swap them back out again
     * if there are no other allocations using the referenced pages.
     *
     * @param ptr   Pointer to the memory being deallocated.
     * @param len   Number of bytes in the memory being deallocated.
     */
    void remove_allocation(void* ptr, std::size_t len);

    /**
     * @brief
     * Record a new memory allocation.
     *
     * This will lock the referenced pages of memory so that the OS will not swap them out if the
     * pages have not already be marked as such.
     *
     * @param ptr   Pointer to the newly allocated memory.
     * @param len   Number of bytes in the newly allocated memory.
     */
    void serialized_add_allocation(void* ptr, std::size_t len)
    {
        std::lock_guard lk{_mutex};
        add_allocation(ptr, len);
    }

    /**
     * @brief
     * Process the deallocation of a memory region.
     *
     * This will unlock the referenced pages of memory so that the OS can swap them back out again
     * if there are no other allocations using the referenced pages.
     *
     * @param ptr   Pointer to the memory being deallocated.
     * @param len   Number of bytes in the memory being deallocated.
     */
    void serialized_remove_allocation(void* ptr, std::size_t len)
    {
        std::lock_guard lk{_mutex};
        remove_allocation(ptr, len);
    }

#ifdef EC_UNIT_TEST_SUPPORT
    /**
     * This method exists only to aid certain unit tests to ensure that all tests can start from a
     * known state.  All this does is remove all the pages in the specified memory region without
     * calling the OS unlock API.
     *
     * @param ptr   Pointer to the memory to be removed from tracking.
     * @param len   Number of bytes in the memory to be removed from tracking.
     */
    void clear_pages(void* ptr, std::size_t len);

    /**
     * This method exists only to aid certain unit tests to check if the mutex is locked or not.
     *
     * @return  An indication of whether the mutex is locked or not.
     */
    bool is_lock_held();
#endif

  private:
    /// @brief The signleton pointer to ourself.
    static std::shared_ptr<no_swap_allocator_state> _self;

    /// @brief A mapping of page pointers and the number of allocations in those pages.
    std::unordered_map<void*, std::uint32_t> _page_ref_count;

    /// @brief Mutex to protect _page_ref_count in multi-threaded applications.
    mutable std::mutex _mutex;

    /**
     * @brief
     * Page size of the system.  Only need to get this once, so store the result in a variable.
     */
    const std::size_t _page_size;

    /**
     * @brief
     * The real default constructor - made private to prevent accidental instantiation by others.
     */
    no_swap_allocator_state();

    /**
     * @brief
     * Helper function to get the page a pointer exists in.
     *
     * Apparently, good, old-fashioned bit masking is not liked by modern C++ compilers.  The STL
     * provides a function called `std::align()` which can be made to do the trick but it has some
     * odd side effects.
     *
     * @param ptr   Pointer to convert to a base page address.
     *
     * @return  Pointer to the page containing ptr;
     */
    std::byte* to_page(void* ptr);

    template <typename, typename>
    friend class serialized_no_swap_allocator;

    template <typename, typename>
    friend class unserialized_no_swap_allocator;
};

} // namespace details

/**
 * @brief
 * This is a C++ STL compatible allocator adapter that will ensure that the allocated memory does
 * not get swapped out to disk until after it is deallocated.
 *
 * This version accesses shared global state information without an kind of serialization (i.e.,
 * mutex locking).  As such, it is only suitable for single threaded applications.
 *
 * @code
 * if (condition) {
 *     std::basic_string<char, std::char_traits<char>, ec::unserialized_no_swap_allocator<char>> password = read_from_console();
 *     // password locked to RAM.
 *     process_password(password);  // Takes a std::string_view.
 *  }  // password destroyed - memory can be swapped again.
 * @endcode
 *
 * Important note: Some care must be taken with certain containers.  For example, `std::string` is
 *                 allowed to use an optimization called Short String Optimization (SSO).  This
 *                 means that for strings below a certain length, the `std::string` implementation
 *                 may not actually allocate any memory.  Such strings can still be swapped out.
 *                 Whether or not SSO is employed and what the maximum length of a "short string"
 *                 is, is implementation defined.
 *
 * @tparam T    The type being allocated.
 * @tparam A    The actual allocator being wrapped.
 */
template <typename T, typename A = std::allocator<T>>
struct unserialized_no_swap_allocator {
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
    using propagate_on_container_copy_assignment = typename upstream_traits::propagate_on_container_copy_assignment;
    /// @brief Compile-time indication about how to handle the allocator when moving containers.
    using propagate_on_container_move_assignment = typename upstream_traits::propagate_on_container_move_assignment;
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
        using other = unserialized_no_swap_allocator<U, typename upstream_traits::template rebind_alloc<U, Us...>>;
    };

    /// @brief Default constructor.
    unserialized_no_swap_allocator() = default;
    /// @brief Move constructor.
    unserialized_no_swap_allocator(unserialized_no_swap_allocator&&) = default;
    /// @brief Copy constructor.
    unserialized_no_swap_allocator(const unserialized_no_swap_allocator&) = default;

    /**
     * @brief
     * Constructor to move from an alternate allocation type.
     *
     * @tparam Ts   The type parameters for the alternate form to move from.
     *
     * @param other     The allocator being moved from.
     */
    template <typename... Ts>
    unserialized_no_swap_allocator(unserialized_no_swap_allocator<Ts...>&& other):
        _upstream_allocator(std::move(other._upstream_allocator))
    {}

    /**
     * @brief
     * Constructor to copy from an alternate allocation type.
     *
     * @tparam Ts   The type parameters for the alternate form to copy from.
     *
     * @param other     The allocator being copied from.
     */
    template <typename... Ts>
    unserialized_no_swap_allocator(const unserialized_no_swap_allocator<Ts...>& other):
        _upstream_allocator(other._upstream_allocator)
    {}

    /**
     * @brief
     * Allocate the requested amount of memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
    EC_NODISCARD
    T* allocate(std::size_t len)
    {
        T* ptr = _upstream_allocator.allocate(len);
        _state->add_allocation(ptr, len * sizeof(T));
        return ptr;
    }

#if defined(__cpp_lib_allocate_at_least)
    /**
     * @brief
     * Allocate at least the requested amount of memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
    EC_NODISCARD
    auto allocate_at_least(std::size_t len)
    {
        auto r = _upstream_allocator.allocate_at_least(len);
        _state->add_allocation(r.ptr, r.count);
        return r;
    }
#endif

    /**
     * @brief
     * Deallocate a block of memory.
     *
     * Important note: This will unlock the memory so that it can be swapped before the upstream
     * allocator is invoked to deallocate the memory.
     *
     * @param ptr   Address of the memory to be deallcoated.
     * @param len   Number of type T to deallocate.
     */
    void deallocate(T* ptr, std::size_t len)
    {
        _state->remove_allocation(ptr, len * sizeof(T));
        _upstream_allocator.deallocate(ptr, len);
    }

  private:
    /// @brief Shared pointer to the allocated pages state.
    std::shared_ptr<details::no_swap_allocator_state> _state{details::no_swap_allocator_state::get_state_object()};
    upstream_allocator _upstream_allocator{};    ///< @brief The real allocator that will manage the actual memory allocations.

    template <typename, typename>
    friend class unserialized_no_swap_allocator;
};

/**
 * @brief
 * This is a C++ STL compatible allocator adapter that will ensure that the allocated memory does
 * not get swapped out to disk until after it is deallocated.
 *
 * This version accesses shared global state information with serialization (i.e., mutex locking).
 * As such, it is safe to use in multi-threaded applications.
 *
 * @code
 * if (condition) {
 *     std::basic_string<char, std::char_traits<char>, ec::serialized_no_swap_allocator<char>> password = read_from_console();
 *     // password locked to RAM.
 *     process_password(password);  // Takes a std::string_view.
 *  }  // password destroyed - memory can be swapped again.
 * @endcode
 *
 * Important note: Some care must be taken with certain containers.  For example, `std::string` is
 *                 allowed to use an optimization called Short String Optimization (SSO).  This
 *                 means that for strings below a certain length, the `std::string` implementation
 *                 may not actually allocate any memory.  Such strings can still be swapped out.
 *                 Whether or not SSO is employed and what the maximum length of a "short string"
 *                 is, is implementation defined.
 *
 * @tparam T    The type being allocated.
 * @tparam A    The actual allocator being wrapped.
 */
template <typename T, typename A = std::allocator<T>>
struct serialized_no_swap_allocator {
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
    using propagate_on_container_copy_assignment = typename upstream_traits::propagate_on_container_copy_assignment;
    /// @brief Compile-time indication about how to handle the allocator when moving containers.
    using propagate_on_container_move_assignment = typename upstream_traits::propagate_on_container_move_assignment;
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
    template <typename U>
    struct rebind {
        /// @brief The rebound allocator type.
        using other = serialized_no_swap_allocator<U, typename upstream_traits::template rebind_alloc<U>>;
    };

    /// @brief Default constructor.
    serialized_no_swap_allocator() = default;
    /// @brief Move constructor.
    serialized_no_swap_allocator(serialized_no_swap_allocator&&) = default;
    /// @brief Copy constructor.
    serialized_no_swap_allocator(const serialized_no_swap_allocator&) = default;

    /**
     * @brief
     * Constructor to move from an alternate allocation type.
     *
     * @tparam U    The alternate type for the allocator being moved from.
     *
     * @param other     The allocator being moved from.
     */
    template <typename U>
    serialized_no_swap_allocator(serialized_no_swap_allocator<U>&& other):
        _upstream_allocator(std::move(other._upstream_allocator))
    {}

    /**
     * @brief
     * Constructor to copy from an alternate allocation type.
     *
     * @tparam U    The alternate type for the allocator being copied from.
     *
     * @param other     The allocator being copied from.
     */
    template <typename U>
    serialized_no_swap_allocator(const serialized_no_swap_allocator<U>& other):
        _upstream_allocator(other._upstream_allocator)
    {}

#if __has_cpp_attribute(nodiscard)
    /**
     * @brief
     * Allocate the requested amount of memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
    [[nodiscard]]
#endif
    T* allocate(std::size_t len)
    {
        T* ptr = _upstream_allocator.allocate(len);
        _state->serialized_add_allocation(ptr, len * sizeof(T));
        return ptr;
    }

    /**
     * @brief
     * Allocate at least the requested amount of memory.
     *
     * @param len   Number of type T to allocate.
     *
     * @return  Address of the allocated memory.
     */
#if defined(__cpp_lib_allocate_at_least)
#if __has_cpp_attribute(nodiscard)
    [[nodiscard]]
#endif
    auto* allocate_at_least(std::size_t len)
    {
        auto r = _upstream_allocator.allocate_at_least(len);
        _state->serialized_add_allocation(r.ptr, r.count);
        return r;
    }
#endif

    /**
     * @brief
     * Deallocate a block of memory.
     *
     * Important note: This will unlock the memory so that it can be swapped before the upstream
     * allocator is invoked to deallocate the memory.
     *
     * @param ptr   Address of the memory to be deallcoated.
     * @param len   Number of type T to deallocate.
     */
    void deallocate(T* ptr, std::size_t len)
    {
        _state->serialized_remove_allocation(ptr, len * sizeof(T));
        _upstream_allocator.deallocate(ptr, len);
    }

  private:
    /// @brief Shared pointer to the allocated pages state.
    std::shared_ptr<details::no_swap_allocator_state> _state{details::no_swap_allocator_state::get_state_object()};
    upstream_allocator _upstream_allocator{};    ///< @brief The real allocator that will manage the actual memory allocations.

    template <typename, typename>
    friend class serialized_no_swap_allocator;
};

} // namespace ec
