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

#include "hpp_pipeline_layout.h"

#include <core/hpp_device.h>
#include <core/hpp_shader_module.h>

namespace vkb
{
    namespace core
    {
        pipeline_layout::pipeline_layout(vkb::core::device& device, const std::vector<vkb::core::shader_module*>& shader_modules) :
            device_{device},
            shader_modules_{shader_modules}
        {
            // Collect and combine all the shader resources from each of the shader modules
            // Collate them all into a map that is indexed by the name of the resource
            for (auto* shader_module : shader_modules)
            {
                for (const auto& shader_resource : shader_module->get_resources())
                {
                    std::string key = shader_resource.name;

                    // Since 'Input' and 'Output' resources can have the same name, we modify the key string
                    if (shader_resource.type == vkb::core::shader_resource_type::Input || shader_resource.type == vkb::core::shader_resource_type::Output)
                    {
                        key = std::to_string(static_cast<uint32_t>(shader_resource.stages)) + "_" + key;
                    }

                    auto it = shader_resources_.find(key);

                    if (it != shader_resources_.end())
                    {
                        // Append stage flags if resource already exists
                        it->second.stages |= shader_resource.stages;
                    }
                    else
                    {
                        // Create a new entry in the map
                        shader_resources_.emplace(key, shader_resource);
                    }
                }
            }

            // Sift through the map of name indexed shader resources
            // Separate them into their respective sets
            for (auto& it : shader_resources_)
            {
                auto& shader_resource = it.second;

                // Find binding by set index in the map.
                auto it2 = shader_sets_.find(shader_resource.set);

                if (it2 != shader_sets_.end())
                {
                    // Add resource to the found set index
                    it2->second.push_back(shader_resource);
                }
                else
                {
                    // Create a new set index and with the first resource
                    shader_sets_.emplace(shader_resource.set, std::vector<vkb::core::shader_resource>{shader_resource});
                }
            }

            // Create a descriptor set layout for each shader set in the shader modules
            for (auto& shader_set_it : shader_sets_)
            {
                descriptor_set_layouts_.emplace_back(
                    &device.get_resource_cache().request_descriptor_set_layout(shader_set_it.first, shader_modules, shader_set_it.second));
            }

            // Collect all the descriptor set layout handles, maintaining set order
            std::vector<vk::DescriptorSetLayout> descriptor_set_layout_handles;
            for (auto descriptor_set_layout : descriptor_set_layouts_)
            {
                descriptor_set_layout_handles.push_back(descriptor_set_layout ? descriptor_set_layout->get_handle() : nullptr);
            }

            // Collect all the push constant shader resources
            std::vector<vk::PushConstantRange> push_constant_ranges;
            for (auto& push_constant_resource : resources(vkb::core::shader_resource_type::PushConstant))
            {
                push_constant_ranges.push_back({push_constant_resource.stages, push_constant_resource.offset, push_constant_resource.size});
            }

            vk::PipelineLayoutCreateInfo create_info({}, descriptor_set_layout_handles, push_constant_ranges);

            // Create the Vulkan pipeline layout handle
            handle_ = device.get_handle().createPipelineLayout(create_info);
        }

        pipeline_layout::pipeline_layout(pipeline_layout&& other) :
            device_{other.device_},
            handle_{other.handle_},
            shader_modules_{std::move(other.shader_modules_)},
            shader_resources_{std::move(other.shader_resources_)},
            shader_sets_{std::move(other.shader_sets_)},
            descriptor_set_layouts_{std::move(other.descriptor_set_layouts_)}
        {
            other.handle_ = nullptr;
        }

        pipeline_layout::~pipeline_layout()
        {
            if (handle_)
            {
                device_.get_handle().destroyPipelineLayout(handle_);
            }
        }

        vkb::core::descriptor_set_layout const& pipeline_layout::descriptor_set_layout(const uint32_t set_index) const
        {
            auto it = std::find_if(descriptor_set_layouts_.begin(),
                                   descriptor_set_layouts_.end(),
                                   [&set_index](auto const* descriptor_set_layout) { return descriptor_set_layout->get_index() == set_index; });
            if (it == descriptor_set_layouts_.end())
            {
                throw std::runtime_error("Couldn't find descriptor set layout at set index " + to_string(set_index));
            }
            return **it;
        }

        vk::PipelineLayout pipeline_layout::handle() const
        {
            return handle_;
        }

        vk::ShaderStageFlags pipeline_layout::push_constant_range_stage(uint32_t size, uint32_t offset) const
        {
            vk::ShaderStageFlags stages;

            for (auto& push_constant_resource : resources(vkb::core::shader_resource_type::PushConstant))
            {
                if (push_constant_resource.offset <= offset && offset + size <= push_constant_resource.offset + push_constant_resource.size)
                {
                    stages |= push_constant_resource.stages;
                }
            }
            return stages;
        }

        std::vector<vkb::core::shader_resource> pipeline_layout::resources(const vkb::core::shader_resource_type& type, vk::ShaderStageFlagBits stage) const
        {
            std::vector<vkb::core::shader_resource> found_resources;

            for (auto& it : shader_resources_)
            {
                auto& shader_resource = it.second;

                if (shader_resource.type == type || type == vkb::core::shader_resource_type::All)
                {
                    if (shader_resource.stages == stage || stage == vk::ShaderStageFlagBits::eAll)
                    {
                        found_resources.push_back(shader_resource);
                    }
                }
            }

            return found_resources;
        }

        const std::vector<vkb::core::shader_module*>& pipeline_layout::shader_modules() const
        {
            return shader_modules_;
        }

        const std::unordered_map<uint32_t, std::vector<vkb::core::shader_resource>>& pipeline_layout::shader_sets() const
        {
            return shader_sets_;
        }

        bool pipeline_layout::has_descriptor_set_layout(const uint32_t set_index) const
        {
            return set_index < descriptor_set_layouts_.size();
        }
    } // namespace core
} // namespace vkb
