/**
 * @file
 * Unit tests for C++ allocator that is usable by STL containers that blocks allocated memory from
 * being swapped to disk.
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

#include "mock_allocator.h"
#include "mock_c_lib.h"
#include "mock_memory.h"

#include <deque>
#include <fmt/format.h>
#include <forward_list>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

// Compile-time compatibility with STL containers.  Also, may test construction/destruction ordering
// of globals with the allocation tracker data structure.
using uut_allocator = ec::unserialized_no_swap_allocator<int>;
std::vector<int, uut_allocator>       test_vector;
std::deque<int, uut_allocator>        test_deque;
std::forward_list<int, uut_allocator> test_forward_list;
std::list<int, uut_allocator>         test_list;

std::set<int, std::less<int>, uut_allocator>      test_set;
std::map<int, std::less<int>, uut_allocator>      test_map;
std::multiset<int, std::less<int>, uut_allocator> test_multiset;
std::multimap<int, std::less<int>, uut_allocator> test_multimap;

std::unordered_set<int, std::hash<int>, std::equal_to<int>, uut_allocator> test_unordered_set;
std::unordered_map<int, std::hash<int>, std::equal_to<int>, uut_allocator> test_unordered_map;
std::unordered_multiset<int, std::hash<int>, std::equal_to<int>, uut_allocator>
    test_unordered_multiset;
std::unordered_multimap<int, std::hash<int>, std::equal_to<int>, uut_allocator>
    test_unordered_multimap;

std::stack<int, std::deque<int, uut_allocator>>           test_stack;
std::queue<int, std::vector<int, uut_allocator>>          test_queue;
std::priority_queue<int, std::vector<int, uut_allocator>> test_priority_queue;


using ::testing::_;
using ::testing::Return;

template <typename TypeParam>
class unserialized_no_swap_allocator_typed_test: public ::testing::Test {
  protected:
    const std::size_t             page_size{static_cast<std::size_t>(sysconf(_SC_PAGESIZE))};
    std::shared_ptr<mock::memory> memory{mock::memory::get_instance()};
    std::shared_ptr<mock::allocation_monitor> mock_allocator{
        mock::allocation_monitor::get_instance()};
    std::shared_ptr<mock::c_lib> mock_c_lib{mock::c_lib::get_instance()};
    std::unique_ptr<
        ec::unserialized_no_swap_allocator<TypeParam, mock::monitored_allocator<TypeParam>>>
        allocator;

    virtual void SetUp() {
        memory->reset();
        allocator = std::make_unique<
            ec::unserialized_no_swap_allocator<TypeParam, mock::monitored_allocator<TypeParam>>>();
        auto& mem = memory->get_memory_array();
        ec::details::no_swap_allocator_state::get_state_object()->clear_pages(mem.data(),
                                                                              mem.size());
    }

    virtual void TearDown() { allocator.reset(); }

    template <typename... P>
    void test_allocate(std::size_t alloc_offset, std::size_t alloc_size, P... page_num) {
        auto memory_base = memory->get_memory_array().data();
        auto alloc_addr  = reinterpret_cast<TypeParam*>(memory_base + alloc_offset);

        EXPECT_CALL(*mock_allocator, void_allocate(alloc_size * sizeof(TypeParam)));
        if (sizeof...(P) == 0) {
            EXPECT_CALL(*mock_c_lib, mlock(_, _)).Times(0);
        } else {
            (EXPECT_CALL(*mock_c_lib, mlock(memory_base + (page_size * page_num), page_size))
                 .WillOnce(Return(0)),
             ...);
        }

        memory->set_next_allocation_offset(alloc_offset);
        auto addr = allocator->allocate(alloc_size);
        EXPECT_EQ(addr, alloc_addr);
    }

    template <typename... P>
    void test_deallocate(std::size_t alloc_offset, std::size_t alloc_size, P... page_num) {
        auto memory_base = memory->get_memory_array().data();
        auto alloc_addr  = reinterpret_cast<TypeParam*>(memory_base + alloc_offset);

        EXPECT_CALL(*mock_allocator, void_deallocate(alloc_addr, alloc_size * sizeof(TypeParam)));
        if (sizeof...(P) == 0) {
            EXPECT_CALL(*mock_c_lib, munlock(_, _)).Times(0);
        } else {
            (EXPECT_CALL(*mock_c_lib, munlock(memory_base + (page_size * page_num), page_size))
                 .WillOnce(Return(0)),
             ...);
        }
        allocator->deallocate(alloc_addr, alloc_size);
    }
};

#define TEST_ALLOCATE(_alloc_offset, _alloc_size, ...)                               \
    do {                                                                             \
        SCOPED_TRACE(fmt::format("Test Allocate: alloc_addr = {}   alloc_size = {}", \
                                 _alloc_offset, _alloc_size));                       \
        this->test_allocate(_alloc_offset, _alloc_size, ##__VA_ARGS__);              \
    } while (false)

#define TEST_DEALLOCATE(_alloc_offset, _alloc_size, ...)                               \
    do {                                                                               \
        SCOPED_TRACE(fmt::format("Test Deallocate: alloc_addr = {}   alloc_size = {}", \
                                 _alloc_offset, _alloc_size));                         \
        this->test_deallocate(_alloc_offset, _alloc_size, ##__VA_ARGS__);              \
    } while (false)


using test_types = ::testing::Types<std::uint8_t, std::uint32_t>;
TYPED_TEST_SUITE(unserialized_no_swap_allocator_typed_test, test_types);

TYPED_TEST(unserialized_no_swap_allocator_typed_test, page_aligned_full_page) {
    auto alloc_size   = this->page_size / sizeof(TypeParam);
    auto alloc_offset = 0;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0);
    TEST_DEALLOCATE(alloc_offset, alloc_size, 0);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, page_aligned_single_element) {
    auto alloc_size   = 1;
    auto alloc_offset = 0;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0);
    TEST_DEALLOCATE(alloc_offset, alloc_size, 0);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, unaligned_single_element) {
    auto alloc_size   = 1;
    auto alloc_offset = 16;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0);
    TEST_DEALLOCATE(alloc_offset, alloc_size, 0);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, aligned_two_page_array) {
    auto alloc_size   = 2 * this->page_size / sizeof(TypeParam);
    auto alloc_offset = 0;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0, 1);
    TEST_DEALLOCATE(alloc_offset, alloc_size, 0, 1);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, unaligned_cross_page_array) {
    auto alloc_size   = this->page_size / sizeof(TypeParam);
    auto alloc_offset = (this->page_size / 2);

    TEST_ALLOCATE(alloc_offset, alloc_size, 0, 1);
    TEST_DEALLOCATE(alloc_offset, alloc_size, 0, 1);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, two_adjacent_allocations) {
    auto alloc_size    = 1;
    auto alloc_offset1 = 0;
    auto alloc_offset2 = sizeof(TypeParam) * 1;

    TEST_ALLOCATE(alloc_offset1, alloc_size, 0);
    TEST_ALLOCATE(alloc_offset2, alloc_size);
    TEST_DEALLOCATE(alloc_offset1, alloc_size);
    TEST_DEALLOCATE(alloc_offset2, alloc_size, 0);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, two_disjoint_allocations_in_same_page) {
    auto alloc_size    = 1;
    auto alloc_offset1 = 0;
    auto alloc_offset2 = sizeof(TypeParam) * 8;

    TEST_ALLOCATE(alloc_offset1, alloc_size, 0);
    TEST_ALLOCATE(alloc_offset2, alloc_size);
    TEST_DEALLOCATE(alloc_offset1, alloc_size);
    TEST_DEALLOCATE(alloc_offset2, alloc_size, 0);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, two_disjoint_allocations_in_separate_pages) {
    auto alloc_size    = 1;
    auto alloc_offset1 = 0;
    auto alloc_offset2 = this->page_size;

    TEST_ALLOCATE(alloc_offset1, alloc_size, 0);
    TEST_ALLOCATE(alloc_offset2, alloc_size, 1);
    TEST_DEALLOCATE(alloc_offset1, alloc_size, 0);
    TEST_DEALLOCATE(alloc_offset2, alloc_size, 1);
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, mlock_failed) {
    auto alloc_size = 1;

    EXPECT_CALL(*this->mock_allocator, void_allocate(_));

    EXPECT_CALL(*this->mock_c_lib, mlock(_, _)).WillOnce(Return(-1));

    errno = EINVAL;
    EXPECT_THROW((void)this->allocator->allocate(alloc_size), std::runtime_error);
    errno = 0;
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, munlock_failed) {
    auto alloc_size   = 1;
    auto alloc_offset = 0;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0);

    // Can't expect the real allocator's deallocate() method to be called since the exception will
    // be thrown before it would be called.
    // EXPECT_CALL(*this->allocator_mock, deallocate(alloc_addr, alloc_size))
    //     .WillOnce(Return());

    EXPECT_CALL(*this->mock_c_lib, munlock(_, _)).WillOnce(Return(-1));

    auto memory_base = this->memory->get_memory_array().data();
    auto alloc_addr  = reinterpret_cast<TypeParam*>(memory_base + alloc_offset);

    errno = EINVAL;
    EXPECT_THROW(this->allocator->deallocate(alloc_addr, alloc_size), std::runtime_error);
    errno = 0;
}

TYPED_TEST(unserialized_no_swap_allocator_typed_test, deallocate_past_end_of_allocated_space) {
    auto alloc_size   = 1;
    auto alloc_offset = 0;

    TEST_ALLOCATE(alloc_offset, alloc_size, 0);

    auto memory_base = this->memory->get_memory_array().data();
    auto alloc_addr  = reinterpret_cast<TypeParam*>(memory_base + alloc_offset);

    errno = EINVAL;
    EXPECT_THROW(this->allocator->deallocate(alloc_addr + this->page_size, alloc_size),
                 std::runtime_error);
    errno = 0;
}

TEST(serialized_no_swap_allocator_test, lock_held_during_allocate_and_deallocate) {
    const std::size_t page_size{static_cast<std::size_t>(sysconf(_SC_PAGESIZE))};
    auto              memory{mock::memory::get_instance()};
    auto              mock_c_lib = mock::c_lib::get_instance();
    auto              allocator =
        std::make_unique<ec::serialized_no_swap_allocator<int, mock::monitored_allocator<int>>>();
    auto  mock_allocator{mock::allocation_monitor::get_instance()};
    auto& state_obj = *ec::details::no_swap_allocator_state::get_state_object();
    state_obj.clear_pages(memory->get_memory_array().data(), memory->get_memory_array().size());

    int lock_count{};

    auto lock_count_action = [&lock_count, &state_obj](const void*, std::size_t) {
        if (state_obj.is_lock_held()) { ++lock_count; }
        return 0;
    };

    EXPECT_CALL(*mock_allocator, void_allocate(_));
    EXPECT_CALL(*mock_c_lib, mlock(_, _)).WillOnce(lock_count_action);
    auto addr = allocator->allocate(1);

    EXPECT_EQ(lock_count, 1);

    EXPECT_CALL(*mock_allocator, void_deallocate(_, _));
    EXPECT_CALL(*mock_c_lib, munlock(_, _)).WillOnce(lock_count_action);
    allocator->deallocate(addr, 1);

    EXPECT_EQ(lock_count, 2);
}
