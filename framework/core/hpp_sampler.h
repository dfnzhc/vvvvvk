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

#include "core/hpp_vulkan_resource.h"

namespace vkb
{
    namespace core
    {
        /**
         * @brief Represents a Vulkan Sampler, using Vulkan-Hpp
         */
        class sampler : public vkb::core::vk_unit<vk::Sampler>
        {
        public:
            sampler(vkb::core::device& device, const vk::SamplerCreateInfo& info);

            sampler(sampler&& sampler);

            ~sampler();

            sampler(const sampler&) = delete;
            sampler& operator=(const sampler&) = delete;
            sampler& operator=(sampler&&) = delete;
        };
    } // namespace core
} // namespace vkb
