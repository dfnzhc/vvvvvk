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

#include "rendering/hpp_render_target.h"

#include "core/hpp_device.h"

namespace vkb
{
    namespace rendering
    {
        const render_target::CreateFunc render_target::DEFAULT_CREATE_FUNC = [](core::image&& swapchain_image) -> std::unique_ptr<render_target>
        {
            vk::Format depth_format = common::get_suitable_depth_format(swapchain_image.get_device().get_gpu().get_handle());

            core::image depth_image{
                swapchain_image.get_device(), swapchain_image.extent(),
                depth_format,
                vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
                VMA_MEMORY_USAGE_GPU_ONLY
            };

            std::vector<core::image> images;
            images.push_back(std::move(swapchain_image));
            images.push_back(std::move(depth_image));

            return std::make_unique<render_target>(std::move(images));
        };

        render_target::render_target(std::vector<core::image>&& images_) :
            device_{images_.back().get_device()},
            images_{std::move(images_)}
        {
            assert(!images_.empty() && "Should specify at least 1 image");

            // check that every image is 2D
            auto it = std::find_if(images_.begin(), images_.end(), [](core::image const& image) { return image.type() != vk::ImageType::e2D; });
            if (it != images_.end())
            {
                throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Image type is not 2D"};
            }

            extent_.width = images_.front().extent().width;
            extent_.height = images_.front().extent().height;

            // check that every image has the same extent
            it = std::find_if(std::next(images_.begin()),
                              images_.end(),
                              [this](core::image const& image) { return (extent_.width != image.extent().width) || (extent_.height != image.extent().height); });
            if (it != images_.end())
            {
                throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Extent size is not unique"};
            }

            for (auto& image : images_)
            {
                views_.emplace_back(image, vk::ImageViewType::e2D);
                attachments_.emplace_back(attachment{image.format(), image.sample_count(), image.usage()});
            }
        }

        render_target::render_target(std::vector<core::image_view>&& image_views) :
            device_{image_views.back().image().get_device()},
            views_{std::move(image_views)}
        {
            assert(!views_.empty() && "Should specify at least 1 image view");

            const uint32_t mip_level = views_.front().subresource_range().baseMipLevel;
            extent_.width = views_.front().image().extent().width >> mip_level;
            extent_.height = views_.front().image().extent().height >> mip_level;

            // check that every image view has the same extent
            auto it = std::find_if(std::next(views_.begin()),
                                   views_.end(),
                                   [this](core::image_view const& image_view)
                                   {
                                       const uint32_t mip_level = image_view.subresource_range().baseMipLevel;
                                       return (extent_.width != image_view.image().extent().width >> mip_level) ||
                                           (extent_.height != image_view.image().extent().height >> mip_level);
                                   });
            if (it != views_.end())
            {
                throw VulkanException{VK_ERROR_INITIALIZATION_FAILED, "Extent size is not unique"};
            }

            for (auto& view : views_)
            {
                const auto& image = view.image();
                attachments_.emplace_back(attachment{image.format(), image.sample_count(), image.usage()});
            }
        }

        const vk::Extent2D& render_target::extent() const
        {
            return extent_;
        }

        const std::vector<core::image_view>& render_target::views() const
        {
            return views_;
        }

        const std::vector<attachment>& render_target::attachments() const
        {
            return attachments_;
        }

        void render_target::set_input_attachments(std::vector<uint32_t>& input)
        {
            input_attachments_ = input;
        }

        const std::vector<uint32_t>& render_target::input_attachments() const
        {
            return input_attachments_;
        }

        void render_target::set_output_attachments(std::vector<uint32_t>& output)
        {
            output_attachments_ = output;
        }

        const std::vector<uint32_t>& render_target::output_attachments() const
        {
            return output_attachments_;
        }

        void render_target::set_layout(uint32_t attachment, vk::ImageLayout layout)
        {
            attachments_[attachment].initial_layout = layout;
        }

        vk::ImageLayout render_target::layout(uint32_t attachment) const
        {
            return attachments_[attachment].initial_layout;
        }
    } // namespace rendering
} // namespace vkb
