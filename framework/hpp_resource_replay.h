/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
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

#include "resource_replay.h"

namespace vkb
{
    class resource_cache;
    class resource_record;

    /**
     * @brief facade class around vkb::ResourceReplay, providing a vulkan.hpp-based interface
     *
     * See vkb::ResourceReplay for documentation
     */
    class resource_replay : private vkb::ResourceReplay
    {
    public:
        void play(vkb::resource_cache& resource_cache, vkb::resource_record& recorder)
        {
            vkb::ResourceReplay::play(reinterpret_cast<vkb::ResourceCache&>(resource_cache), reinterpret_cast<vkb::ResourceRecord&>(recorder));
        }
    };
} // namespace vkb
