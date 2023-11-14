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

#include "core/hpp_image.h"
#include <functional>
#include <memory>

namespace vkb
{
    namespace core
    {
        class device;
    }

    namespace rendering
    {
        struct attachment
        {
            attachment() = default;

            attachment(vk::Format format, vk::SampleCountFlagBits samples, vk::ImageUsageFlags usage) :
                format{format},
                samples{samples},
                usage{usage}
            {
            }

            vk::Format format = vk::Format::eUndefined;
            vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
            vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
            vk::ImageLayout initial_layout = vk::ImageLayout::eUndefined;
        };

        class render_target
        {
        public:
            using CreateFunc = std::function<std::unique_ptr<render_target>(core::image&&)>;

            static const CreateFunc DEFAULT_CREATE_FUNC;

            render_target(std::vector<core::image>&& images);
            render_target(std::vector<core::image_view>&& image_views);

            render_target(const render_target&) = delete;
            render_target(render_target&&) = delete;

            render_target& operator=(const render_target& other) noexcept = delete;
            render_target& operator=(render_target&& other) noexcept = delete;

            const vk::Extent2D& extent() const;
            const std::vector<core::image_view>& views() const;
            const std::vector<attachment>& attachments() const;

            void set_input_attachments(std::vector<uint32_t>& input);
            const std::vector<uint32_t>& input_attachments() const;

            void set_output_attachments(std::vector<uint32_t>& output);
            const std::vector<uint32_t>& output_attachments() const;

            void set_layout(uint32_t attachment, vk::ImageLayout layout);
            vk::ImageLayout layout(uint32_t attachment) const;

        private:
            core::device const& device_;
            vk::Extent2D extent_;
            std::vector<core::image> images_;
            std::vector<core::image_view> views_;
            std::vector<attachment> attachments_;
            std::vector<uint32_t> input_attachments_ = {}; // By default there are no input attachments
            std::vector<uint32_t> output_attachments_ = {0}; // By default the output attachments is attachment 0
        };
    } // namespace rendering
} // namespace vkb
