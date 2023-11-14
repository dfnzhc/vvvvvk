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

#pragma once

#include <core/hpp_command_buffer.h>
#include <cstdint>

namespace vkb
{
    namespace rendering
    {
        class HPPRenderFrame;
    }

    namespace core
    {
        class device;

        class command_pool
        {
        public:
            command_pool(device& device,
                         uint32_t queue_family_index,
                         vkb::rendering::HPPRenderFrame* render_frame = nullptr,
                         size_t thread_index = 0,
                         HPPCommandBuffer::ResetMode reset_mode = HPPCommandBuffer::ResetMode::ResetPool);
            command_pool(const command_pool&) = delete;
            command_pool(command_pool&& other);
            ~command_pool();

            command_pool& operator=(const command_pool&) = delete;
            command_pool& operator=(command_pool&&) = delete;

            device& get_device();
            vk::CommandPool get_handle() const;
            uint32_t get_queue_family_index() const;
            vkb::rendering::HPPRenderFrame* get_render_frame();
            HPPCommandBuffer::ResetMode get_reset_mode() const;
            size_t get_thread_index() const;
            HPPCommandBuffer& request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
            void reset_pool();

        private:
            void reset_command_buffers();

        private:
            device& device_;
            vk::CommandPool handle_ = nullptr;
            vkb::rendering::HPPRenderFrame* render_frame_ = nullptr;
            size_t thread_index_ = 0;
            uint32_t queue_family_index_ = 0;
            std::vector<std::unique_ptr<HPPCommandBuffer>> primary_command_buffers_;
            uint32_t active_primary_command_buffer_count_ = 0;
            std::vector<std::unique_ptr<HPPCommandBuffer>> secondary_command_buffers_;
            uint32_t active_secondary_command_buffer_count_ = 0;
            HPPCommandBuffer::ResetMode reset_mode_ = HPPCommandBuffer::ResetMode::ResetPool;
        };
    } // namespace core
} // namespace vkb