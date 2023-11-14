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

#include "core/hpp_image.h"
#include "core/hpp_vulkan_resource.h"

namespace vkb
{
    namespace core
    {
        class image_view : public vkb::core::vk_unit<vk::ImageView>
        {
        public:
            image_view(vkb::core::image& image,
                       vk::ImageViewType view_type,
                       vk::Format format = vk::Format::eUndefined,
                       uint32_t base_mip_level = 0,
                       uint32_t base_array_layer = 0,
                       uint32_t n_mip_levels = 0,
                       uint32_t n_array_layers = 0);

            image_view(image_view&& other);

            ~image_view() override;

            image_view(image_view&) = delete;
            image_view& operator=(const image_view&) = delete;
            image_view& operator=(image_view&&) = delete;

            vk::Format format() const;
            vkb::core::image const& image() const;
            void set_image(vkb::core::image& image);
            vk::ImageSubresourceLayers subresource_layers() const;
            vk::ImageSubresourceRange subresource_range() const;

        private:
            vkb::core::image* image_ = nullptr;
            vk::Format format_;
            vk::ImageSubresourceRange subresource_range_;
        };
    } // namespace core
} // namespace vkb
