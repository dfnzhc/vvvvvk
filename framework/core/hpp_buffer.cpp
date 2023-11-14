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

#include "hpp_buffer.h"

#include "hpp_device.h"

namespace vkb
{
    namespace core
    {
        HPPBuffer::HPPBuffer(vkb::core::device& device,
                             vk::DeviceSize size_,
                             vk::BufferUsageFlags buffer_usage,
                             VmaMemoryUsage memory_usage,
                             VmaAllocationCreateFlags flags,
                             const std::vector<uint32_t>& queue_family_indices) :
            vk_unit(nullptr, &device), size_(size_)
        {
#ifdef VK_USE_PLATFORM_METAL_EXT
	// Workaround for Mac (MoltenVK requires unmapping https://github.com/KhronosGroup/MoltenVK/issues/175)
	// Force cleares the flag VMA_ALLOCATION_CREATE_MAPPED_BIT
	flags &= ~VMA_ALLOCATION_CREATE_MAPPED_BIT;
#endif

            persistent_ = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

            vk::BufferCreateInfo buffer_create_info({}, size_, buffer_usage);
            if (queue_family_indices.size() >= 2)
            {
                buffer_create_info.sharingMode = vk::SharingMode::eConcurrent;
                buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
                buffer_create_info.pQueueFamilyIndices = queue_family_indices.data();
            }

            VmaAllocationCreateInfo memory_info{};
            memory_info.flags = flags;
            memory_info.usage = memory_usage;

            VmaAllocationInfo allocation_info{};
            auto result = vmaCreateBuffer(device.get_memory_allocator(),
                                          reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &memory_info,
                                          reinterpret_cast<VkBuffer*>(&get_handle()), &allocation_,
                                          &allocation_info);

            if (result != VK_SUCCESS)
            {
                throw VulkanException{result, "Cannot create HPPBuffer"};
            }

            memory_ = static_cast<vk::DeviceMemory>(allocation_info.deviceMemory);

            if (persistent_)
            {
                mapped_data_ = static_cast<uint8_t*>(allocation_info.pMappedData);
            }
        }

        HPPBuffer::HPPBuffer(HPPBuffer&& other) :
            vk_unit{other.get_handle(), &other.get_device()},
            allocation_(std::exchange(other.allocation_, {})),
            memory_(std::exchange(other.memory_, {})),
            size_(std::exchange(other.size_, {})),
            mapped_data_(std::exchange(other.mapped_data_, {})),
            mapped_(std::exchange(other.mapped_, {}))
        {
        }

        HPPBuffer::~HPPBuffer()
        {
            if (get_handle() && (allocation_ != VK_NULL_HANDLE))
            {
                unmap();
                vmaDestroyBuffer(get_device().get_memory_allocator(), static_cast<VkBuffer>(get_handle()), allocation_);
            }
        }

        VmaAllocation HPPBuffer::allocation() const
        {
            return allocation_;
        }

        vk::DeviceMemory HPPBuffer::memory() const
        {
            return memory_;
        }

        vk::DeviceSize HPPBuffer::size() const
        {
            return size_;
        }

        const uint8_t* HPPBuffer::data() const
        {
            return mapped_data_;
        }

        uint8_t* HPPBuffer::map()
        {
            if (!mapped_ && !mapped_data_)
            {
                VK_CHECK(vmaMapMemory(get_device().get_memory_allocator(), allocation_, reinterpret_cast<void **>(&mapped_data_)));
                mapped_ = true;
            }
            return mapped_data_;
        }

        void HPPBuffer::unmap()
        {
            if (mapped_)
            {
                vmaUnmapMemory(get_device().get_memory_allocator(), allocation_);
                mapped_data_ = nullptr;
                mapped_ = false;
            }
        }

        void HPPBuffer::flush()
        {
            vmaFlushAllocation(get_device().get_memory_allocator(), allocation_, 0, size_);
        }

        void HPPBuffer::update(const std::vector<uint8_t>& data, size_t offset)
        {
            update(data.data(), data.size(), offset);
        }

        uint64_t HPPBuffer::device_address() const
        {
            return get_device().get_handle().getBufferAddressKHR({get_handle()});
        }

        void HPPBuffer::update(void* data, size_t size, size_t offset)
        {
            update(reinterpret_cast<const uint8_t*>(data), size, offset);
        }

        void HPPBuffer::update(const uint8_t* data, const size_t size, const size_t offset)
        {
            if (persistent_)
            {
                std::copy(data, data + size, mapped_data_ + offset);
                flush();
            }
            else
            {
                map();
                std::copy(data, data + size, mapped_data_ + offset);
                flush();
                unmap();
            }
        }
    } // namespace core
} // namespace vkb
