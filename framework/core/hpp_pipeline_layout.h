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

#include <core/hpp_shader_module.h>
#include <vector>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class device;
        class descriptor_set_layout;

        /**
         * @brief A wrapper class for vk::HPPPipelineLayout
         *
         */
        class pipeline_layout
        {
        public:
            pipeline_layout(vkb::core::device& device, const std::vector<vkb::core::shader_module*>& shader_modules);
            pipeline_layout(pipeline_layout&& other);
            ~pipeline_layout();

            pipeline_layout(const pipeline_layout&) = delete;
            pipeline_layout& operator=(const pipeline_layout&) = delete;
            pipeline_layout& operator=(pipeline_layout&&) = delete;

            vkb::core::descriptor_set_layout const& descriptor_set_layout(const uint32_t set_index) const;
            vk::PipelineLayout handle() const;
            vk::ShaderStageFlags push_constant_range_stage(uint32_t size, uint32_t offset = 0) const;
            std::vector<vkb::core::shader_resource> resources(const vkb::core::shader_resource_type& type = vkb::core::shader_resource_type::All,
                                                                vk::ShaderStageFlagBits stage = vk::ShaderStageFlagBits::eAll) const;
            const std::vector<vkb::core::shader_module*>& shader_modules() const;
            const std::unordered_map<uint32_t, std::vector<vkb::core::shader_resource>>& shader_sets() const;
            bool has_descriptor_set_layout(const uint32_t set_index) const;

        private:
            vkb::core::device& device_;
            vk::PipelineLayout handle_;
            std::vector<vkb::core::shader_module*> shader_modules_;
            std::unordered_map<std::string, vkb::core::shader_resource> shader_resources_;
            std::unordered_map<uint32_t, std::vector<vkb::core::shader_resource>> shader_sets_;
            std::vector<vkb::core::descriptor_set_layout*> descriptor_set_layouts_;
        };
    } // namespace core
} // namespace vkb
