/**
 * @file
 * A collection of simple tests of the secure allocator as used with STL containers.
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

#include <enhanced_containers/secure_deque.h>
#include <enhanced_containers/secure_forward_list.h>
#include <enhanced_containers/secure_list.h>
#include <enhanced_containers/secure_map.h>
#include <enhanced_containers/secure_multimap.h>
#include <enhanced_containers/secure_multiset.h>
#include <enhanced_containers/secure_set.h>
#include <enhanced_containers/secure_string.h>
#include <enhanced_containers/secure_unordered_map.h>
#include <enhanced_containers/secure_unordered_multimap.h>
#include <enhanced_containers/secure_unordered_multiset.h>
#include <enhanced_containers/secure_unordered_set.h>
#include <enhanced_containers/secure_vector.h>

#include "mock_allocator.h"
#include "mock_c_lib.h"
#include "mock_memory.h"

#include <cstddef>
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iterator>
#include <type_traits>

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnArg;

namespace {

template <typename C>
    requires requires(C& c, C::value_type& v) {
        { *std::back_inserter(c) = v };
        { c.push_back(v) };
    }
auto test_inserter(C& c) {
    return std::back_inserter(c);
}

template <typename C>
    requires requires(C& c, C::value_type& v) {
        { *std::front_inserter(c) = v };
        { c.push_front(v) };
    } && (!requires(C& c, C::value_type& v) {
                 { *std::back_inserter(c) = v };
                 { c.push_back(v) };
             })
auto test_inserter(C& c) {
    return std::front_inserter(c);
}

template <typename C>
    requires requires(C& c, C::value_type& v) {
        { *std::inserter(c, c.end()) = v };
        { c.insert(v) };
    } &&
             (!(
                 requires(C& c, C::value_type& v) {
                     { *std::front_inserter(c) = v };
                     { c.push_front(v) };
                 } ||
                 requires(C& c, C::value_type& v) {
                     { *std::back_inserter(c) = v };
                     { c.push_back(v) };
                 }))
auto test_inserter(C& c) {
    return std::inserter(c, c.end());
}

template <typename C>
struct test_value_generator {
    std::uint32_t v{1};

    auto operator()() {
        if constexpr (requires(C::value_type& v) {
                          { v++ };
                      }) {
            return v++;
        } else {
            return typename C::value_type{v++, v++};
        }
    }
};

}    // namespace

template <typename TypeParam>
class secure_containers_typed_test: public ::testing::Test {
  protected:
    std::shared_ptr<mock::memory>             memory{mock::memory::get_instance()};
    std::shared_ptr<mock::allocation_monitor> mock_allocator{
        mock::allocation_monitor::get_instance()};
    std::shared_ptr<mock::c_lib> mock_c_lib{mock::c_lib::get_instance()};

    virtual void SetUp() {
        auto& mem = memory->get_memory_array();
        memory->reset();
        EXPECT_CALL(*mock_c_lib, memset(mem.data(), _, mem.size()))
            .WillOnce(Return(memory->get_memory_array().data()));
        memory->fill();
        ec::details::no_swap_allocator_state::get_state_object()->clear_pages(mem.data(),
                                                                              mem.size());
    }

    virtual void TearDown() {}

    bool is_memory_zeroed_out(std::size_t offset = 0, std::size_t len = mock::memory::memory_size) {
        const auto& mem       = memory->get_memory_array();
        auto        is_zeroed = [](const auto& v) { return v == std::byte{0}; };
        return (std::all_of(std::next(mem.begin(), offset), std::next(mem.begin(), offset + len),
                            is_zeroed));
    }

    std::string report_memory(std::size_t offset, std::size_t len) {
        enum class value_type { INIT, ZERO, NON_ZERO } current_value_type{value_type::INIT};
        auto        mem = memory->get_memory_array();
        std::string r;

        for (std::size_t i = 0; i < mem.size(); ++i) {
            if (mem[i] == static_cast<std::byte>(0) && current_value_type != value_type::ZERO) {
                if (r.empty()) {
                    r = fmt::format("Zeroed byte offset range: [{},  ", i);
                } else {
                    r = fmt::format("{}{})\nZeroed byte offset range: [{}, ", r, i, i);
                }
                current_value_type = value_type::ZERO;
            } else if (mem[i] != static_cast<std::byte>(0) &&
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
                        mem.size() - 1, offset, offset + len,
                        fmt::join(std::next(mem.begin(), offset),
                                  std::next(mem.begin(), offset + len), ", "));
        return r;
    }
};

using monitored_allocator = mock::monitored_allocator<std::uint32_t>;
using pair_monitored_allocator =
    mock::monitored_allocator<std::pair<const std::uint32_t, std::uint32_t>>;

using test_types = ::testing::Types<
    ec::unserialized_secure::vector<std::uint32_t, monitored_allocator>,
    ec::unserialized_secure::deque<std::uint32_t, monitored_allocator>,
    ec::unserialized_secure::forward_list<std::uint32_t, monitored_allocator>,
    ec::unserialized_secure::list<std::uint32_t, monitored_allocator>,

    ec::unserialized_secure::set<std::uint32_t, std::less<std::uint32_t>, monitored_allocator>,
    ec::unserialized_secure::map<std::uint32_t, std::uint32_t, std::less<std::uint32_t>,
                                 pair_monitored_allocator>,
    ec::unserialized_secure::multiset<std::uint32_t, std::less<std::uint32_t>, monitored_allocator>,
    ec::unserialized_secure::multimap<std::uint32_t, std::uint32_t, std::less<std::uint32_t>,
                                      pair_monitored_allocator>,

    ec::unserialized_secure::unordered_set<std::uint32_t, std::hash<std::uint32_t>,
                                           std::equal_to<std::uint32_t>, monitored_allocator>,
    ec::unserialized_secure::unordered_map<std::uint32_t, std::uint32_t, std::hash<std::uint32_t>,
                                           std::equal_to<std::uint32_t>, pair_monitored_allocator>,
    ec::unserialized_secure::unordered_multiset<std::uint32_t, std::hash<std::uint32_t>,
                                                std::equal_to<std::uint32_t>, monitored_allocator>,
    ec::unserialized_secure::unordered_multimap<
        std::uint32_t, std::uint32_t, std::hash<std::uint32_t>, std::equal_to<std::uint32_t>,
        pair_monitored_allocator>>;

TYPED_TEST_SUITE(secure_containers_typed_test, test_types);

TYPED_TEST(secure_containers_typed_test, basic_usage) {
    std::size_t len{};
    {
        EXPECT_CALL(*this->mock_allocator, void_allocate(_))
            .WillRepeatedly([&len, this](std::size_t n) {
                len += n;
                return reinterpret_cast<std::uint32_t*>(this->memory->acquire(n));
            });
        EXPECT_CALL(*this->mock_c_lib, mlock(_, _)).WillRepeatedly(Return(0));

        TypeParam container;

        EXPECT_CALL(*this->mock_c_lib, munlock(_, _)).WillRepeatedly(Return(0));
        EXPECT_CALL(*this->mock_c_lib, memset(_, 0, _)).WillRepeatedly(ReturnArg<0>());
        EXPECT_CALL(*this->mock_allocator, void_deallocate(_, _)).WillRepeatedly(Return());

        std::generate_n(test_inserter(container), 32, test_value_generator<TypeParam>{});
        EXPECT_FALSE(this->is_memory_zeroed_out(0, len));
    }
    EXPECT_TRUE(this->is_memory_zeroed_out(0, len)) << this->report_memory(0, len);
}
