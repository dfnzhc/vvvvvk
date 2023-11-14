/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
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

#include <core/hpp_command_pool.h>
#include <core/hpp_device.h>

namespace vkb
{
    namespace core
    {
        command_pool::command_pool(device& d,
                                       uint32_t queue_family_index,
                                       vkb::rendering::HPPRenderFrame* render_frame,
                                       size_t thread_index,
                                       HPPCommandBuffer::ResetMode reset_mode) :
            device_{d}, render_frame_{render_frame}, thread_index_{thread_index}, reset_mode_{reset_mode}
        {
            vk::CommandPoolCreateFlags flags;
            switch (reset_mode)
            {
            case HPPCommandBuffer::ResetMode::ResetIndividually:
            case HPPCommandBuffer::ResetMode::AlwaysAllocate:
                flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
                break;
            case HPPCommandBuffer::ResetMode::ResetPool:
            default:
                flags = vk::CommandPoolCreateFlagBits::eTransient;
                break;
            }

            vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);

            handle_ = device_.get_handle().createCommandPool(command_pool_create_info);
        }

        command_pool::command_pool(command_pool&& other) :
            device_(other.device_),
            handle_(std::exchange(other.handle_, {})),
            queue_family_index_(std::exchange(other.queue_family_index_, {})),
            primary_command_buffers_{std::move(other.primary_command_buffers_)},
            active_primary_command_buffer_count_(std::exchange(other.active_primary_command_buffer_count_, {})),
            secondary_command_buffers_{std::move(other.secondary_command_buffers_)},
            active_secondary_command_buffer_count_(std::exchange(other.active_secondary_command_buffer_count_, {})),
            render_frame_(std::exchange(other.render_frame_, {})),
            thread_index_(std::exchange(other.thread_index_, {})),
            reset_mode_(std::exchange(other.reset_mode_, {}))
        {
        }

        command_pool::~command_pool()
        {
            // clear command buffers before destroying the command pool
            primary_command_buffers_.clear();
            secondary_command_buffers_.clear();

            // Destroy command pool
            if (handle_)
            {
                device_.get_handle().destroyCommandPool(handle_);
            }
        }

        device& command_pool::get_device()
        {
            return device_;
        }

        vk::CommandPool command_pool::get_handle() const
        {
            return handle_;
        }

        uint32_t command_pool::get_queue_family_index() const
        {
            return queue_family_index_;
        }

        vkb::rendering::HPPRenderFrame* command_pool::get_render_frame()
        {
            return render_frame_;
        }

        size_t command_pool::get_thread_index() const
        {
            return thread_index_;
        }

        void command_pool::reset_pool()
        {
            switch (reset_mode_)
            {
            case HPPCommandBuffer::ResetMode::ResetIndividually:
                reset_command_buffers();
                break;

            case HPPCommandBuffer::ResetMode::ResetPool:
                device_.get_handle().resetCommandPool(handle_);
                reset_command_buffers();
                break;

            case HPPCommandBuffer::ResetMode::AlwaysAllocate:
                primary_command_buffers_.clear();
                active_primary_command_buffer_count_ = 0;
                secondary_command_buffers_.clear();
                active_secondary_command_buffer_count_ = 0;
                break;

            default:
                throw std::runtime_error("Unknown reset mode for command pools");
            }
        }

        HPPCommandBuffer& command_pool::request_command_buffer(vk::CommandBufferLevel level)
        {
            if (level == vk::CommandBufferLevel::ePrimary)
            {
                if (active_primary_command_buffer_count_ < primary_command_buffers_.size())
                {
                    return *primary_command_buffers_[active_primary_command_buffer_count_++];
                }

                primary_command_buffers_.emplace_back(std::make_unique<HPPCommandBuffer>(*this, level));

                active_primary_command_buffer_count_++;

                return *primary_command_buffers_.back();
            }
            else
            {
                if (active_secondary_command_buffer_count_ < secondary_command_buffers_.size())
                {
                    return *secondary_command_buffers_[active_secondary_command_buffer_count_++];
                }

                secondary_command_buffers_.emplace_back(std::make_unique<HPPCommandBuffer>(*this, level));

                active_secondary_command_buffer_count_++;

                return *secondary_command_buffers_.back();
            }
        }

        HPPCommandBuffer::ResetMode command_pool::get_reset_mode() const
        {
            return reset_mode_;
        }

        void command_pool::reset_command_buffers()
        {
            for (auto& cmd_buf : primary_command_buffers_)
            {
                cmd_buf->reset(reset_mode_);
            }
            active_primary_command_buffer_count_ = 0;

            for (auto& cmd_buf : secondary_command_buffers_)
            {
                cmd_buf->reset(reset_mode_);
            }
            active_secondary_command_buffer_count_ = 0;
        }
    } // namespace core
} // namespace vkb
