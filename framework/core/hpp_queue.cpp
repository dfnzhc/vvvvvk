/* Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_queue.h>

#include <core/hpp_command_buffer.h>
#include <core/hpp_device.h>

namespace vkb
{
    namespace core
    {
        queue::queue(vkb::core::device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present, uint32_t index) :
            device_{device},
            family_index_{family_index},
            index_{index},
            can_present_{can_present},
            properties_{properties}
        {
            handle_ = device.get_handle().getQueue(family_index, index);
        }

        queue::queue(queue&& other) :
            device_(other.device_),
            handle_(std::exchange(other.handle_, {})),
            family_index_(std::exchange(other.family_index_, {})),
            index_(std::exchange(other.index_, 0)),
            can_present_(std::exchange(other.can_present_, false)),
            properties_(std::exchange(other.properties_, {}))
        {
        }

        const device& queue::get_device() const
        {
            return device_;
        }

        vk::Queue queue::get_handle() const
        {
            return handle_;
        }

        uint32_t queue::get_family_index() const
        {
            return family_index_;
        }

        uint32_t queue::get_index() const
        {
            return index_;
        }

        const vk::QueueFamilyProperties& queue::get_properties() const
        {
            return properties_;
        }

        vk::Bool32 queue::support_present() const
        {
            return can_present_;
        }

        void queue::submit(const command_buffer& command_buffer, vk::Fence fence) const
        {
            vk::CommandBuffer commandBuffer = command_buffer.get_handle();
            vk::SubmitInfo submit_info({}, {}, commandBuffer);
            handle_.submit(submit_info, fence);
        }

        vk::Result queue::present(const vk::PresentInfoKHR& present_info) const
        {
            if (!can_present_)
            {
                return vk::Result::eErrorIncompatibleDisplayKHR;
            }

            return handle_.presentKHR(present_info);
        }
    } // namespace core
} // namespace vkb
