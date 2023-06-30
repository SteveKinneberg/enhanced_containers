/**
 * @file
 * Mock STL allocator.
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

#include "mock_allocator.h"

namespace mock {
std::shared_ptr<allocation_monitor> allocation_monitor::_self;

std::shared_ptr<allocation_monitor> allocation_monitor::get_instance()
{
    if (!_self) {
        _self.reset(new allocation_monitor{});
    }
    return _self;
}

}
