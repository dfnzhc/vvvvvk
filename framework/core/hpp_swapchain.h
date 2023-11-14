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

#include <set>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class device;

        struct HPPSwapchainProperties
        {
            vk::SwapchainKHR old_swapchain;
            uint32_t image_count{3};
            vk::Extent2D extent;
            vk::SurfaceFormatKHR surface_format;
            uint32_t array_layers;
            vk::ImageUsageFlags image_usage;
            vk::SurfaceTransformFlagBitsKHR pre_transform;
            vk::CompositeAlphaFlagBitsKHR composite_alpha;
            vk::PresentModeKHR present_mode;
        };

        class swapchain
        {
        public:
            swapchain(swapchain& old_swapchain, const vk::Extent2D& extent);
            swapchain(swapchain& old_swapchain, const uint32_t image_count);
            swapchain(swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags);
            swapchain(swapchain& swapchain, const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

            swapchain(device& device,
                      vk::SurfaceKHR surface,
                      const vk::PresentModeKHR present_mode,
                      const std::vector<vk::PresentModeKHR>& present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
                      const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list = {
                          {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
                          {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
                      },
                      const vk::Extent2D& extent = {},
                      const uint32_t image_count = 3,
                      const vk::SurfaceTransformFlagBitsKHR transform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
                      const std::set<vk::ImageUsageFlagBits>& image_usage_flags = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
                      vk::SwapchainKHR old_swapchain = nullptr);


            swapchain(swapchain&& other);

            ~swapchain();

            swapchain(const swapchain&) = delete;
            swapchain& operator=(const swapchain&) = delete;
            swapchain& operator=(swapchain&&) = delete;

            bool is_valid() const;

            device const& get_device() const;
            vk::SwapchainKHR handle() const;
            vk::SurfaceKHR surface() const;

            std::pair<vk::Result, uint32_t> acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const;

            const vk::Extent2D& extent() const;
            vk::Format format() const;

            const std::vector<vk::Image>& images() const;
            vk::SurfaceTransformFlagBitsKHR transform() const;

            vk::ImageUsageFlags usage() const;
            vk::PresentModeKHR present_mode() const;

        private:
            device& device_;

            vk::SurfaceKHR surface_;
            vk::SwapchainKHR handle_;

            std::vector<vk::Image> images_;

            std::vector<vk::SurfaceFormatKHR> surface_formats_;
            std::vector<vk::PresentModeKHR> present_modes_;

            HPPSwapchainProperties properties_;

            std::vector<vk::PresentModeKHR> present_mode_priority_list_;
            std::vector<vk::SurfaceFormatKHR> surface_format_priority_list_;

            std::set<vk::ImageUsageFlagBits> image_usage_flags_;
        };
    } // namespace core
} // namespace vkb
