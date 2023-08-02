/**
 * @file
 * Unit tests for C++ allocator usable by STL containers that zeros out memory on destruction.
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

#include <enhanced_containers/zero_on_release_allocator.h>

#include "mock_allocator.h"

#include <algorithm>
#include <deque>
#include <fmt/format.h>
#include <forward_list>
#include <iterator>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>
#include <unordered_set>

// Compile-time compatibility with STL containers.  Also, may test construction/destruction ordering
// of globals with the allocation tracker data structure.
using uut_allocator = ec::zero_on_release_allocator<int>;
std::vector<int, uut_allocator>       test_vector;
std::deque<int, uut_allocator>        test_deque;
std::forward_list<int, uut_allocator> test_forward_list;
std::list<int, uut_allocator>         test_list;

std::set<int, uut_allocator>      test_set;
std::map<int, uut_allocator>      test_map;
std::multiset<int, uut_allocator> test_multiset;
std::multimap<int, uut_allocator> test_multimap;

std::unordered_set<int, uut_allocator>      test_unordered_set;
std::unordered_map<int, uut_allocator>      test_unordered_map;
std::unordered_multiset<int, uut_allocator> test_unordered_multiset;
std::unordered_multimap<int, uut_allocator> test_unordered_multimap;

std::stack<int, std::deque<int, uut_allocator>>           test_stack;
std::queue<int, std::vector<int, uut_allocator>>          test_queue;
std::priority_queue<int, std::vector<int, uut_allocator>> test_priority_queue;

using ::testing::_;
using ::testing::DoDefault;

class zero_on_release_allocator_test: public ::testing::Test {
  protected:
    std::shared_ptr<mock::memory>             memory{mock::memory::get_instance()};
    std::shared_ptr<mock::allocation_monitor> mock_allocator{
        mock::allocation_monitor::get_instance()};
    static constexpr std::size_t test_offset{64};

    virtual void SetUp() {
        memory->reset();
        memory->fill();
        memory->set_next_allocation_offset(test_offset);
    }

    bool is_memory_zeroed_out(std::size_t offset, std::size_t len) {
        const auto& data      = memory->get_memory_array();
        auto        is_zeroed = [](const auto& v) { return v == std::byte{0}; };
        return (std::none_of(data.begin(), std::next(data.begin(), offset), is_zeroed) &&
                std::all_of(std::next(data.begin(), offset), std::next(data.begin(), offset + len),
                            is_zeroed) &&
                std::none_of(std::next(data.begin(), offset + len), data.end(), is_zeroed));
    }

    std::string report_memory(std::size_t offset, std::size_t len) {
        enum class value_type { INIT, ZERO, NON_ZERO } current_value_type{value_type::INIT};
        auto&       data = memory->get_memory_array();
        std::string r;

        for (std::size_t i = 0; i < data.size(); ++i) {
            if (data[i] == static_cast<std::byte>(0) && current_value_type != value_type::ZERO) {
                if (r.empty()) {
                    r = fmt::format("Zeroed byte offset range:     [{},  ", i);
                } else {
                    r = fmt::format("{}{})\nZeroed byte offset range:     [{}, ", r, i, i);
                }
                current_value_type = value_type::ZERO;
            } else if (data[i] != static_cast<std::byte>(0) &&
                       current_value_type != value_type::NON_ZERO) {
                if (r.empty()) {
                    r = fmt::format("Non-Zeroed byte offset range: [{}, ", i);
                } else {
                    r = fmt::format("{}{})\nNon-Zeroed byte offset range: [{}, ", r, i, i);
                }
                current_value_type = value_type::NON_ZERO;
            }
        }
        r = fmt::format("{}{})\nExpected zeored byte range to be [{}, {}): {:02x}", r,
                        data.size() - 1, offset, offset + len,
                        fmt::join(std::next(data.begin(), offset),
                                  std::next(data.begin(), offset + len), ", "));
        return r;
    }
};

TEST_F(zero_on_release_allocator_test, one_element) {
    std::size_t len{};
    auto&       data = memory->get_memory_array();
    {
        std::vector<int, ec::zero_on_release_allocator<int, mock::monitored_allocator<int>>> v;
        EXPECT_CALL(*this->mock_allocator, void_allocate(_)).WillOnce([&len, this](std::size_t n) {
            len += n;
            return reinterpret_cast<void*>(memory->acquire(n));
        });
        EXPECT_CALL(*this->mock_allocator, void_deallocate(_, _));
        v.emplace_back(0x12345678);
        EXPECT_FALSE(is_memory_zeroed_out(test_offset, len));
    }

    EXPECT_TRUE(is_memory_zeroed_out(test_offset, len)) << report_memory(test_offset, len);
}

TEST_F(zero_on_release_allocator_test, many_elements) {
    std::size_t len{};
    {
        std::vector<int, ec::zero_on_release_allocator<int, mock::monitored_allocator<int>>> v;
        EXPECT_CALL(*this->mock_allocator, void_allocate(_))
            .WillRepeatedly([&len, this](std::size_t n) {
                len += n;
                return reinterpret_cast<int*>(memory->acquire(n));
            });
        EXPECT_CALL(*this->mock_allocator, void_deallocate(_, _)).WillRepeatedly(DoDefault());
        std::fill_n(std::back_inserter(v), 1000, 0xffff);
        EXPECT_FALSE(is_memory_zeroed_out(test_offset, len));
    }

    EXPECT_TRUE(is_memory_zeroed_out(test_offset, len)) << report_memory(test_offset, len);
}
