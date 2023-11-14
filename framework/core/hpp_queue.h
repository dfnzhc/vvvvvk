/* Copyright (c) 2021-2022, NVIDIA CORPORATION. All rights reserved.
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

#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class HPPCommandBuffer;
        class device;

        class queue
        {
        public:
            queue(device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index);
            
            queue(const queue&) = default;
            queue(queue&& other);

            queue& operator=(const queue&) = delete;
            queue& operator=(queue&&) = delete;

            const device& get_device() const;
            vk::Queue get_handle() const;

            uint32_t get_family_index() const;
            uint32_t get_index() const;

            const vk::QueueFamilyProperties& get_properties() const;
            vk::Bool32 support_present() const;

            void submit(const HPPCommandBuffer& command_buffer, vk::Fence fence) const;
            vk::Result present(const vk::PresentInfoKHR& present_infos) const;

        private:
            device& device_;
            vk::Queue handle_;

            uint32_t family_index_{0};
            uint32_t index_{0};

            vk::Bool32 can_present_ = false;

            vk::QueueFamilyProperties properties_{};
        };
    } // namespace core
} // namespace vkb
