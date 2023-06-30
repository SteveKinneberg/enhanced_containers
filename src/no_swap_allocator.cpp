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

#include <enhanced_containers/no_swap_allocator.h>

#include <unordered_map>


#if defined(__linux__) || defined(__unix) || defined(__unix__)
#include <sys/mman.h>
#include <unistd.h>

namespace {
/**
 * @brief
 * Linux implementation to get the number of bytes in a page of memory.
 *
 * @return The number of bytes in a page of memory.
 */
std::size_t get_page_size()
{
    return sysconf(_SC_PAGESIZE);
}

/**
 * @brief
 * Linux implementation to lock a page of memory to RAM (prevent from being swapped out to disk).
 *
 * @param ptr   Pointer to memory in the page to lock.
 * @param len   Number of bytes to lock.  Large enough values will actually cause multiple pages to
 *              be locked.
 */
void pin_memory(const void* ptr, std::size_t len)
{
    int r = mlock(ptr, len);
    if (r != 0) {
        throw std::system_error{errno, std::system_category(), "pinning memory"};
    }
}

/**
 * @brief
 * Linux implementation to unlock a page of memory from RAM (can be swapped out to disk).
 *
 * @param ptr   Pointer to memory in the page to unlock.
 * @param len   Number of bytes to unlock.  Large enough values will actually cause multiple pages to
 *              be unlocked.
 */
void unpin_memory(const void* ptr, std::size_t len)
{
    int r = munlock(ptr, len);
    if (r != 0) {
        throw std::system_error{errno, std::system_category(), "unpinning memory"};
    }
}
}



#elif defined(_WIN32)
#warning Windows support has not been tested.
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <sysinfoapi.h>

namespace {
/**
 * @brief
 * Microsoft Windows implementation to get the number of bytes in a page of memory.
 *
 * @return The number of bytes in a page of memory.
 */
std::size_t get_page_size()
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
}

/**
 * @brief
 * Microsoft Windows implementation to lock a page of memory to RAM (prevent from being swapped out
 * to disk).
 */
void pin_memory(const void* ptr, std::size_t len)
{
    auto r = VirtualLock(ptr, len);
    if (r != 0) {
        throw std::system_error(GetLastError(), "pinning memory");
    }
}

/**
 * @brief
 * Microsoft Windows implementation to unlock a page of memory from RAM (can be swapped out to disk).
 */
void unpin_memory(const void* ptr, std::size_t len)
{
    auto r = VirtualUnlock(ptr, len);
    if (r != 0) {
        throw std::system_error(GetLastError(), "pinning memory");
    }
}
}


#elif defined(__APPLE__) && defined(__MACOS__)
#error Not supported yet.
#endif



namespace ec {


namespace details {

std::shared_ptr<no_swap_allocator_state> no_swap_allocator_state::_self{};

std::shared_ptr<no_swap_allocator_state> no_swap_allocator_state::get_state_object()
{
    if (!_self) {
        _self.reset(new no_swap_allocator_state{});
    }
    return _self;
}

no_swap_allocator_state::no_swap_allocator_state():
    _page_size{get_page_size()}
{}


void no_swap_allocator_state::add_allocation(void* ptr, std::size_t len)
{
    auto bptr = reinterpret_cast<std::byte*>(ptr);
    auto page = to_page(bptr);

    while (page < bptr + len) {
        auto it = _page_ref_count.find(page);
        if (it == _page_ref_count.end()) {
            _page_ref_count.emplace(page, 1);
            pin_memory(page, _page_size);
        } else {
            ++it->second;
        }
        page += _page_size;
    }
}

void no_swap_allocator_state::remove_allocation(void* ptr, std::size_t len)
{
    auto bptr = reinterpret_cast<std::byte*>(ptr);
    auto page = to_page(bptr);

    while (page < bptr + len) {
        auto it = _page_ref_count.find(page);
        if (it == _page_ref_count.end()) {
            throw std::runtime_error("Releasing memory not tracked by no_swap_allocator");
        } else {
            --it->second;
            if (it->second == 0) {
                unpin_memory(page, _page_size);
                _page_ref_count.erase(it);
            }
        }
        page += _page_size;
    }
}

std::byte* no_swap_allocator_state::to_page(void* ptr)
{
    auto fake_buffer_size = 2 * _page_size;
    auto ptr2 = ptr; // Make copy since std::align will modify it.
    auto page = reinterpret_cast<std::byte*>(std::align(_page_size, 1, ptr2, fake_buffer_size));
    if (page > ptr) {
        page -= _page_size;
    }
    return page;
}


#ifdef EC_UNIT_TEST_SUPPORT
void no_swap_allocator_state::clear_pages(void* ptr, std::size_t len)
{
    auto page = to_page(ptr);

    while (page < reinterpret_cast<std::byte*>(ptr) + len) {
        _page_ref_count.erase(page);
        page += _page_size;
    }
}

bool no_swap_allocator_state::is_lock_held()
{
    std::unique_lock lk{_mutex, std::try_to_lock};
    return !lk.owns_lock();
}

#endif


} // namespace detail


} // namespace ec
