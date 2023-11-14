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

#include "core/hpp_image_view.h"

#include "common/hpp_vk_common.h"
#include "core/hpp_device.h"
#include <vulkan/vulkan_format_traits.hpp>

namespace vkb
{
    namespace core
    {
        image_view::image_view(vkb::core::image& img,
                               vk::ImageViewType view_type,
                               vk::Format format,
                               uint32_t mip_level,
                               uint32_t array_layer,
                               uint32_t n_mip_levels,
                               uint32_t n_array_layers) :
            vk_unit{nullptr, &img.get_device()}, image_{&img}, format_{format}
        {
            if (format == vk::Format::eUndefined)
            {
                this->format_ = format = image_->format();
            }

            subresource_range_ =
                vk::ImageSubresourceRange((std::string(vk::componentName(format, 0)) == "D") ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits::eColor,
                                          mip_level,
                                          n_mip_levels == 0 ? image_->subresource().mipLevel : n_mip_levels,
                                          array_layer,
                                          n_array_layers == 0 ? image_->subresource().arrayLayer : n_array_layers);

            vk::ImageViewCreateInfo image_view_create_info({}, image_->get_handle(), view_type, format, {}, subresource_range_);

            set_handle(get_device().get_handle().createImageView(image_view_create_info));

            // Register this image view to its image
            // in order to be notified when it gets moved
            image_->views().emplace(this);
        }

        image_view::image_view(image_view&& other) :
            vk_unit{std::move(other)}, image_{other.image_}, format_{other.format_}, subresource_range_{other.subresource_range_}
        {
            // Remove old view from image set and add this new one
            auto& views = image_->views();
            views.erase(&other);
            views.emplace(this);

            other.set_handle(nullptr);
        }

        image_view::~image_view()
        {
            if (get_handle())
            {
                get_device().get_handle().destroyImageView(get_handle());
            }
        }

        vk::Format image_view::format() const
        {
            return format_;
        }

        const vkb::core::image& image_view::image() const
        {
            assert(image_ && "vkb::core::HPPImage view is referring an invalid image");
            return *image_;
        }

        void image_view::set_image(vkb::core::image& img)
        {
            image_ = &img;
        }

        vk::ImageSubresourceLayers image_view::subresource_layers() const
        {
            return vk::ImageSubresourceLayers(
                subresource_range_.aspectMask, subresource_range_.baseMipLevel, subresource_range_.baseArrayLayer, subresource_range_.layerCount);
        }

        vk::ImageSubresourceRange image_view::subresource_range() const
        {
            return subresource_range_;
        }
    } // namespace core
} // namespace vkb
