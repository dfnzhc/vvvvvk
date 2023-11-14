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

#include "core/hpp_vulkan_resource.h"
#include <unordered_set>
#include <vk_mem_alloc.h>

namespace vkb
{
    namespace core
    {
        class device;
        class image_view;

        class image : public vkb::core::vk_unit<vk::Image>
        {
        public:
            image(device& device,
                  vk::Image handle,
                  const vk::Extent3D& extent,
                  vk::Format format,
                  vk::ImageUsageFlags image_usage,
                  vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

            image(device& device,
                  const vk::Extent3D& extent,
                  vk::Format format,
                  vk::ImageUsageFlags image_usage,
                  VmaMemoryUsage memory_usage,
                  vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1,
                  uint32_t mip_levels = 1,
                  uint32_t array_layers = 1,
                  vk::ImageTiling tiling = vk::ImageTiling::eOptimal,
                  vk::ImageCreateFlags flags = {},
                  uint32_t num_queue_families = 0,
                  const uint32_t* queue_families = nullptr);

            image(image&& other);

            ~image() override;

            image(const image&) = delete;
            image& operator=(const image&) = delete;
            image& operator=(image&&) = delete;

            VmaAllocation memory() const;

            uint8_t* map();
            void unmap();

            vk::ImageType type() const;
            const vk::Extent3D& extent() const;
            vk::Format format() const;
            vk::SampleCountFlagBits sample_count() const;
            vk::ImageUsageFlags usage() const;
            vk::ImageTiling tiling() const;
            vk::ImageSubresource subresource() const;
            uint32_t array_layer_count() const;
            std::unordered_set<vkb::core::image_view*>& views();

        private:
            VmaAllocation memory_ = VK_NULL_HANDLE;
            vk::ImageType type_;
            vk::Extent3D extent_;
            vk::Format format_;
            vk::ImageUsageFlags usage_;
            vk::SampleCountFlagBits sample_count_;
            vk::ImageTiling tiling_;
            vk::ImageSubresource subresource_;
            uint32_t array_layer_count_ = 0;
            std::unordered_set<vkb::core::image_view*> views_; /// HPPImage views referring to this image
            uint8_t* mapped_data_ = nullptr;
            bool mapped_ = false; /// Whether it was mapped with vmaMapMemory
        };
    } // namespace core
} // namespace vkb
