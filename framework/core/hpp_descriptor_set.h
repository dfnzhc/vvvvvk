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

#include "descriptor_set.h"
#include <core/hpp_descriptor_pool.h>
#include <core/hpp_descriptor_set_layout.h>

namespace vkb
{
    namespace core
    {
        /**
         * @brief facade class around vkb::DescriptorSet, providing a vulkan.hpp-based interface
         *
         * See vkb::DescriptorSet for documentation
         */
        class descriptor_set : private vkb::DescriptorSet
        {
        public:
            descriptor_set(vkb::core::device& device,
                           const vkb::core::descriptor_set_layout& descriptor_set_layout,
                           vkb::core::descriptor_pool& descriptor_pool,
                           const BindingMap<vk::DescriptorBufferInfo>& buffer_infos = {},
                           const BindingMap<vk::DescriptorImageInfo>& image_infos = {}) :
                vkb::DescriptorSet(reinterpret_cast<vkb::Device&>(device),
                                   reinterpret_cast<vkb::DescriptorSetLayout const&>(descriptor_set_layout),
                                   reinterpret_cast<vkb::DescriptorPool&>(descriptor_pool),
                                   reinterpret_cast<BindingMap<VkDescriptorBufferInfo> const&>(buffer_infos),
                                   reinterpret_cast<BindingMap<VkDescriptorImageInfo> const&>(image_infos))
            {
            }

            BindingMap<vk::DescriptorBufferInfo>& buffer_infos()
            {
                return reinterpret_cast<BindingMap<vk::DescriptorBufferInfo>&>(vkb::DescriptorSet::get_buffer_infos());
            }

            vk::DescriptorSet handle() const
            {
                return static_cast<vk::DescriptorSet>(vkb::DescriptorSet::get_handle());
            }

            BindingMap<vk::DescriptorImageInfo>& image_infos()
            {
                return reinterpret_cast<BindingMap<vk::DescriptorImageInfo>&>(vkb::DescriptorSet::get_image_infos());
            }

            const vkb::core::descriptor_set_layout& layout() const
            {
                return reinterpret_cast<vkb::core::descriptor_set_layout const&>(vkb::DescriptorSet::get_layout());
            }
        };
    } // namespace core
} // namespace vkb
