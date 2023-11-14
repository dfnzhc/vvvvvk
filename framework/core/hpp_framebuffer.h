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

#include "core/framebuffer.h"
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace rendering
    {
        class render_target;
    }

    namespace core
    {
        class device;
        class render_pass;

        /**
         * @brief facade class around vkb::Framebuffer, providing a vulkan.hpp-based interface
         *
         * See vkb::Framebuffer for documentation
         */
        class framebuffer : private vkb::Framebuffer
        {
        public:
            framebuffer(vkb::core::device& device, const vkb::rendering::render_target& render_target, const vkb::core::render_pass& render_pass) :
                vkb::Framebuffer(reinterpret_cast<vkb::Device&>(device),
                                 reinterpret_cast<vkb::RenderTarget const&>(render_target),
                                 reinterpret_cast<vkb::RenderPass const&>(render_pass))
            {
            }

            const vk::Extent2D& extent() const
            {
                return reinterpret_cast<vk::Extent2D const&>(vkb::Framebuffer::get_extent());
            }

            vk::Framebuffer handle() const
            {
                return static_cast<vk::Framebuffer>(vkb::Framebuffer::get_handle());
            }
        };
    } // namespace core
} // namespace vkb
