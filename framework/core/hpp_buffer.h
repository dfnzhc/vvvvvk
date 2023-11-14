/* Copyright (c) 2021-2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_vulkan_resource.h"

#include <unordered_map>
#include <vk_mem_alloc.h>

namespace vkb
{
    namespace core
    {
        class HPPBuffer : public vkb::core::vk_unit<vk::Buffer>
        {
        public:
            /**
             * @brief Creates a buffer using VMA
             * @param device A valid Vulkan device
             * @param size The size in bytes of the buffer
             * @param buffer_usage The usage flags for the VkBuffer
             * @param memory_usage The memory usage of the buffer
             * @param flags The allocation create flags
             * @param queue_family_indices optional queue family indices
             */
            HPPBuffer(vkb::core::device& device,
                      vk::DeviceSize size,
                      vk::BufferUsageFlags buffer_usage,
                      VmaMemoryUsage memory_usage,
                      VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
                      const std::vector<uint32_t>& queue_family_indices = {});

            HPPBuffer(const HPPBuffer&) = delete;
            HPPBuffer(HPPBuffer&& other);

            ~HPPBuffer();

            HPPBuffer& operator=(const HPPBuffer&) = delete;
            HPPBuffer& operator=(HPPBuffer&&) = delete;

            VmaAllocation allocation() const;
            const uint8_t* data() const;
            vk::DeviceMemory memory() const;

            uint64_t device_address() const;
            vk::DeviceSize size() const;

            void flush();

            uint8_t* map();

            void unmap();

            void update(const uint8_t* data, size_t size, size_t offset = 0);
            void update(void* data, size_t size, size_t offset = 0);
            void update(const std::vector<uint8_t>& data, size_t offset = 0);

            template <class T>
            void convert_and_update(const T& object, size_t offset = 0)
            {
                update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
            }

        private:
            VmaAllocation allocation_ = VK_NULL_HANDLE;
            vk::DeviceMemory memory_ = nullptr;
            vk::DeviceSize size_ = 0;
            uint8_t* mapped_data_ = nullptr;
            bool persistent_ = false;
            bool mapped_ = false;
        };
    } // namespace core
} // namespace vkb
