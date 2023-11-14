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

#include "hpp_resource_cache.h"
#include <common/hpp_resource_caching.h>
#include <core/hpp_descriptor_set.h>
#include <core/hpp_device.h>
#include <core/hpp_image_view.h>
#include <core/hpp_pipeline_layout.h>

namespace vkb
{
    namespace
    {
        template <class T, class... A>
        T& request_resource(
            vkb::core::device& device, vkb::resource_record& recorder, std::mutex& resource_mutex, std::unordered_map<std::size_t, T>& resources, A&... args)
        {
            std::lock_guard<std::mutex> guard(resource_mutex);

            auto& res = request_resource(device, &recorder, resources, args...);

            return res;
        }
    } // namespace

    resource_cache::resource_cache(vkb::core::device& device) :
        device_{device}
    {
    }

    void resource_cache::clear()
    {
        state_.shader_modules.clear();
        state_.pipeline_layouts.clear();
        state_.descriptor_sets.clear();
        state_.descriptor_set_layouts.clear();
        state_.render_passes.clear();
        clear_pipelines();
        clear_framebuffers();
    }

    void resource_cache::clear_framebuffers()
    {
        state_.framebuffers.clear();
    }

    void resource_cache::clear_pipelines()
    {
        state_.graphics_pipelines.clear();
        state_.compute_pipelines.clear();
    }

    const resource_cache_state& resource_cache::get_internal_state() const
    {
        return state_;
    }

    vkb::core::HPPComputePipeline& resource_cache::request_compute_pipeline(vkb::rendering::HPPPipelineState& pipeline_state)
    {
        return request_resource(device_, recorder_, compute_pipeline_mutex, state_.compute_pipelines, pipeline_cache_, pipeline_state);
    }

    vkb::core::HPPDescriptorSet& resource_cache::request_descriptor_set(vkb::core::descriptor_set_layout& descriptor_set_layout,
                                                                        const BindingMap<vk::DescriptorBufferInfo>& buffer_infos,
                                                                        const BindingMap<vk::DescriptorImageInfo>& image_infos)
    {
        auto& descriptor_pool = request_resource(device_, recorder_, descriptor_set_mutex_, state_.descriptor_pools, descriptor_set_layout);
        return request_resource(device_, recorder_, descriptor_set_mutex_, state_.descriptor_sets, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos);
    }

    vkb::core::descriptor_set_layout& resource_cache::request_descriptor_set_layout(const uint32_t set_index,
                                                                                     const std::vector<vkb::core::shader_module*>& shader_modules,
                                                                                     const std::vector<vkb::core::HPPShaderResource>& set_resources)
    {
        return request_resource(device_, recorder_, descriptor_set_layout_mutex_, state_.descriptor_set_layouts, set_index, shader_modules, set_resources);
    }

    vkb::core::HPPFramebuffer& resource_cache::request_framebuffer(const vkb::rendering::render_target& render_target,
                                                                   const vkb::core::HPPRenderPass& render_pass)
    {
        return request_resource(device_, recorder_, framebuffer_mutex_, state_.framebuffers, render_target, render_pass);
    }

    vkb::core::HPPGraphicsPipeline& resource_cache::request_graphics_pipeline(vkb::rendering::HPPPipelineState& pipeline_state)
    {
        return request_resource(device_, recorder_, graphics_pipeline_mutex_, state_.graphics_pipelines, pipeline_cache_, pipeline_state);
    }

    vkb::core::HPPPipelineLayout& resource_cache::request_pipeline_layout(const std::vector<vkb::core::shader_module*>& shader_modules)
    {
        return request_resource(device_, recorder_, pipeline_layout_mutex_, state_.pipeline_layouts, shader_modules);
    }

    vkb::core::HPPRenderPass& resource_cache::request_render_pass(const std::vector<vkb::rendering::attachment>& attachments,
                                                                  const std::vector<vkb::common::HPPLoadStoreInfo>& load_store_infos,
                                                                  const std::vector<vkb::core::HPPSubpassInfo>& subpasses)
    {
        return request_resource(device_, recorder_, render_pass_mutex_, state_.render_passes, attachments, load_store_infos, subpasses);
    }

    vkb::core::shader_module& resource_cache::request_shader_module(vk::ShaderStageFlagBits stage,
                                                                      const vkb::core::shader_source& glsl_source,
                                                                      const vkb::core::shader_variant& shader_variant)
    {
        std::string entry_point{"main"};
        return request_resource(device_, recorder_, shader_module_mutex_, state_.shader_modules, stage, glsl_source, entry_point, shader_variant);
    }

    std::vector<uint8_t> resource_cache::serialize()
    {
        return recorder_.get_data();
    }

    void resource_cache::set_pipeline_cache(vk::PipelineCache new_pipeline_cache)
    {
        pipeline_cache_ = new_pipeline_cache;
    }

    void resource_cache::update_descriptor_sets(const std::vector<vkb::core::image_view>& old_views, const std::vector<vkb::core::image_view>& new_views)
    {
        // Find descriptor sets referring to the old image view
        std::vector<vk::WriteDescriptorSet> set_updates;
        std::set<size_t> matches;

        for (size_t i = 0; i < old_views.size(); ++i)
        {
            auto& old_view = old_views[i];
            auto& new_view = new_views[i];

            for (auto& kd_pair : state_.descriptor_sets)
            {
                auto& key = kd_pair.first;
                auto& descriptor_set = kd_pair.second;

                auto& image_infos = descriptor_set.get_image_infos();

                for (auto& ba_pair : image_infos)
                {
                    auto& binding = ba_pair.first;
                    auto& array = ba_pair.second;

                    for (auto& ai_pair : array)
                    {
                        auto& array_element = ai_pair.first;
                        auto& image_info = ai_pair.second;

                        if (image_info.imageView == old_view.get_handle())
                        {
                            // Save key to remove old descriptor set
                            matches.insert(key);

                            // Update image info with new view
                            image_info.imageView = new_view.get_handle();

                            // Save struct for writing the update later
                            if (auto binding_info = descriptor_set.get_layout().get_layout_binding(binding))
                            {
                                vk::WriteDescriptorSet write_descriptor_set(descriptor_set.get_handle(), binding, array_element, binding_info->descriptorType, image_info);
                                set_updates.push_back(write_descriptor_set);
                            }
                            else
                            {
                                LOGE("Shader layout set does not use image binding at #{}", binding);
                            }
                        }
                    }
                }
            }
        }

        if (!set_updates.empty())
        {
            device_.get_handle().updateDescriptorSets(set_updates, {});
        }

        // Delete old entries (moved out descriptor sets)
        for (auto& match : matches)
        {
            // Move out of the map
            auto it = state_.descriptor_sets.find(match);
            auto descriptor_set = std::move(it->second);
            state_.descriptor_sets.erase(match);

            // Generate new key
            size_t new_key = std::hash<vkb::core::HPPDescriptorSet>()(descriptor_set);

            // Add (key, resource) to the cache
            state_.descriptor_sets.emplace(new_key, std::move(descriptor_set));
        }
    }

    void resource_cache::warmup(const std::vector<uint8_t>& data)
    {
        recorder_.set_data(data);

        replayer_.play(*this, recorder_);
    }
} // namespace vkb
