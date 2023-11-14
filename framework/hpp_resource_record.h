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

#include "resource_record.h"
#include <core/hpp_pipeline.h>

namespace vkb
{
    namespace common
    {
        struct load_store_info;
    }

    namespace rendering
    {
        struct attachment;
    }

    namespace core
    {
        class pipeline_layout;
        class render_pass;
        class shader_module;
        class shader_source;
        class shader_variant;
        struct subpass_info;
    } // namespace core

    /**
     * @brief facade class around vkb::ResourceRecord, providing a vulkan.hpp-based interface
     *
     * See vkb::ResourceRecord for documentation
     */
    class resource_record : private vkb::ResourceRecord
    {
    public:
        using vkb::ResourceRecord::get_data;
        using vkb::ResourceRecord::set_data;

        size_t register_graphics_pipeline(vk::PipelineCache pipeline_cache, vkb::rendering::HPPPipelineState& pipeline_state)
        {
            return vkb::ResourceRecord::register_graphics_pipeline(static_cast<VkPipelineCache>(pipeline_cache),
                                                                   reinterpret_cast<vkb::PipelineState&>(pipeline_state));
        }

        size_t register_pipeline_layout(const std::vector<vkb::core::shader_module*>& shader_modules)
        {
            return vkb::ResourceRecord::register_pipeline_layout(reinterpret_cast<std::vector<vkb::ShaderModule*> const&>(shader_modules));
        }

        size_t register_render_pass(const std::vector<vkb::rendering::attachment>& attachments,
                                    const std::vector<vkb::common::load_store_info>& load_store_infos,
                                    const std::vector<vkb::core::subpass_info>& subpasses)
        {
            return vkb::ResourceRecord::register_render_pass(reinterpret_cast<std::vector<vkb::Attachment> const&>(attachments),
                                                             reinterpret_cast<std::vector<vkb::LoadStoreInfo> const&>(load_store_infos),
                                                             reinterpret_cast<std::vector<vkb::SubpassInfo> const&>(subpasses));
        }

        size_t register_shader_module(vk::ShaderStageFlagBits stage,
                                      const vkb::core::shader_source& glsl_source,
                                      const std::string& entry_point,
                                      const vkb::core::shader_variant& shader_variant)
        {
            return vkb::ResourceRecord::register_shader_module(static_cast<VkShaderStageFlagBits>(stage),
                                                               reinterpret_cast<vkb::ShaderSource const&>(glsl_source),
                                                               entry_point,
                                                               reinterpret_cast<vkb::ShaderVariant const&>(shader_variant));
        }

        void set_graphics_pipeline(size_t index, const vkb::core::graphics_pipeline& graphics_pipeline)
        {
            vkb::ResourceRecord::set_graphics_pipeline(index, reinterpret_cast<vkb::GraphicsPipeline const&>(graphics_pipeline));
        }

        void set_pipeline_layout(size_t index, const vkb::core::pipeline_layout& pipeline_layout)
        {
            vkb::ResourceRecord::set_pipeline_layout(index, reinterpret_cast<vkb::PipelineLayout const&>(pipeline_layout));
        }

        void set_render_pass(size_t index, const vkb::core::render_pass& render_pass)
        {
            vkb::ResourceRecord::set_render_pass(index, reinterpret_cast<vkb::RenderPass const&>(render_pass));
        }

        void set_shader_module(size_t index, const vkb::core::shader_module& shader_module)
        {
            vkb::ResourceRecord::set_shader_module(index, reinterpret_cast<vkb::ShaderModule const&>(shader_module));
        }
    };
} // namespace vkb
