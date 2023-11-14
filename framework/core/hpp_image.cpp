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

#include "core/hpp_image.h"

#include "core/hpp_device.h"

namespace vkb
{
    namespace
    {
        inline vk::ImageType find_image_type(vk::Extent3D const& extent)
        {
            uint32_t dim_num = !!extent.width + !!extent.height + (1 < extent.depth);
            switch (dim_num)
            {
            case 1:
                return vk::ImageType::e1D;
            case 2:
                return vk::ImageType::e2D;
            case 3:
                return vk::ImageType::e3D;
            default:
                throw std::runtime_error("No image type found.");
                return vk::ImageType();
            }
        }
    } // namespace

    namespace core
    {
        image::image(device& device,
                     const vk::Extent3D& extent,
                     vk::Format format,
                     vk::ImageUsageFlags image_usage,
                     VmaMemoryUsage memory_usage,
                     vk::SampleCountFlagBits sample_count,
                     const uint32_t mip_levels,
                     const uint32_t array_layers,
                     vk::ImageTiling tiling,
                     vk::ImageCreateFlags flags,
                     uint32_t num_queue_families,
                     const uint32_t* queue_families) :
            vk_unit{nullptr, &device},
            type_{find_image_type(extent)},
            extent_{extent},
            format_{format},
            sample_count_{sample_count},
            usage_{image_usage},
            array_layer_count_{array_layers},
            tiling_{tiling}
        {
            assert(0 < mip_levels && "HPPImage should have at least one level");
            assert(0 < array_layers && "HPPImage should have at least one layer");

            subresource_.mipLevel = mip_levels;
            subresource_.arrayLayer = array_layers;

            vk::ImageCreateInfo image_info(flags, type_, format, extent, mip_levels, array_layers, sample_count, tiling, image_usage);

            if (num_queue_families != 0)
            {
                image_info.sharingMode = vk::SharingMode::eConcurrent;
                image_info.queueFamilyIndexCount = num_queue_families;
                image_info.pQueueFamilyIndices = queue_families;
            }

            VmaAllocationCreateInfo memory_info{};
            memory_info.usage = memory_usage;

            if (image_usage & vk::ImageUsageFlagBits::eTransientAttachment)
            {
                memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
            }

            auto result = vmaCreateImage(device.get_memory_allocator(),
                                         reinterpret_cast<VkImageCreateInfo const*>(&image_info),
                                         &memory_info,
                                         const_cast<VkImage*>(reinterpret_cast<VkImage const*>(&get_handle())),
                                         &memory_,
                                         nullptr);

            if (result != VK_SUCCESS)
            {
                throw VulkanException{result, "Cannot create HPPImage"};
            }
        }

        image::image(device& device,
                     vk::Image handle,
                     const vk::Extent3D& extent,
                     vk::Format format,
                     vk::ImageUsageFlags image_usage,
                     vk::SampleCountFlagBits sample_count) :
            vk_unit{handle, &device}, type_{find_image_type(extent)}, extent_{extent}, format_{format}, sample_count_{sample_count}, usage_{image_usage}
        {
            subresource_.mipLevel = 1;
            subresource_.arrayLayer = 1;
        }

        image::image(image&& other) :
            vk_unit{std::move(other)},
            memory_(std::exchange(other.memory_, {})),
            type_(std::exchange(other.type_, {})),
            extent_(std::exchange(other.extent_, {})),
            format_(std::exchange(other.format_, {})),
            sample_count_(std::exchange(other.sample_count_, {})),
            usage_(std::exchange(other.usage_, {})),
            tiling_(std::exchange(other.tiling_, {})),
            subresource_(std::exchange(other.subresource_, {})),
            views_(std::exchange(other.views_, {})),
            mapped_data_(std::exchange(other.mapped_data_, {})),
            mapped_(std::exchange(other.mapped_, {}))
        {
            // Update image views references to this image to avoid dangling pointers
            for (auto& view : views_)
            {
                view->set_image(*this);
            }
        }

        image::~image()
        {
            if (get_handle() && memory_)
            {
                unmap();
                vmaDestroyImage(get_device().get_memory_allocator(), static_cast<VkImage>(get_handle()), memory_);
            }
        }

        VmaAllocation image::memory() const
        {
            return memory_;
        }

        uint8_t* image::map()
        {
            if (!mapped_data_)
            {
                if (tiling_ != vk::ImageTiling::eLinear)
                {
                    LOGW("Mapping image memory that is not linear");
                }
                VK_CHECK(vmaMapMemory(get_device().get_memory_allocator(), memory_, reinterpret_cast<void **>(&mapped_data_)));
                mapped_ = true;
            }
            return mapped_data_;
        }

        void image::unmap()
        {
            if (mapped_)
            {
                vmaUnmapMemory(get_device().get_memory_allocator(), memory_);
                mapped_data_ = nullptr;
                mapped_ = false;
            }
        }

        vk::ImageType image::type() const
        {
            return type_;
        }

        const vk::Extent3D& image::extent() const
        {
            return extent_;
        }

        vk::Format image::format() const
        {
            return format_;
        }

        vk::SampleCountFlagBits image::sample_count() const
        {
            return sample_count_;
        }

        vk::ImageUsageFlags image::usage() const
        {
            return usage_;
        }

        vk::ImageTiling image::tiling() const
        {
            return tiling_;
        }

        vk::ImageSubresource image::subresource() const
        {
            return subresource_;
        }

        uint32_t image::array_layer_count() const
        {
            return array_layer_count_;
        }

        std::unordered_set<vkb::core::image_view*>& image::views()
        {
            return views_;
        }
    } // namespace core
} // namespace vkb
