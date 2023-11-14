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

#include "core/hpp_command_buffer.h"
#include <core/hpp_command_pool.h>
#include <core/hpp_device.h>
#include <core/hpp_pipeline.h>
#include <rendering/hpp_render_frame.h>

namespace vkb
{
    namespace core
    {
        command_buffer::command_buffer(vkb::core::command_pool& command_pool, vk::CommandBufferLevel level) :
            vk_unit(nullptr, &command_pool.get_device()),
            level_(level),
            command_pool_(command_pool),
            max_push_constants_size_(get_device().get_gpu().get_properties().limits.maxPushConstantsSize)
        {
            vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);

            set_handle(get_device().get_handle().allocateCommandBuffers(allocate_info).front());
        }

        command_buffer::command_buffer(command_buffer&& other) :
            vk_unit(std::move(other)),
            level_(other.level_),
            command_pool_(other.command_pool_),
            current_render_pass_(std::exchange(other.current_render_pass_, {})),
            pipeline_state_(std::exchange(other.pipeline_state_, {})),
            resource_binding_state_(std::exchange(other.resource_binding_state_, {})),
            stored_push_constants_(std::exchange(other.stored_push_constants_, {})),
            max_push_constants_size_(std::exchange(other.max_push_constants_size_, {})),
            last_framebuffer_extent_(std::exchange(other.last_framebuffer_extent_, {})),
            last_render_area_extent_(std::exchange(other.last_render_area_extent_, {})),
            update_after_bind_(std::exchange(other.update_after_bind_, {})),
            descriptor_set_layout_binding_state_(std::exchange(other.descriptor_set_layout_binding_state_, {}))
        {
        }

        command_buffer::~command_buffer()
        {
            // Destroy command buffer
            if (get_handle())
            {
                get_device().get_handle().freeCommandBuffers(command_pool_.get_handle(), get_handle());
            }
        }

        vk::Result command_buffer::begin(vk::CommandBufferUsageFlags flags, command_buffer* primary_cmd_buf)
        {
            if (level_ == vk::CommandBufferLevel::eSecondary)
            {
                assert(primary_cmd_buf && "A primary command buffer pointer must be provided when calling begin from a secondary one");
                auto const& render_pass_binding = primary_cmd_buf->current_render_pass();

                return begin(flags, render_pass_binding.render_pass, render_pass_binding.framebuffer, primary_cmd_buf->current_subpass_index());
            }

            return begin(flags, nullptr, nullptr, 0);
        }

        vk::Result command_buffer::begin(vk::CommandBufferUsageFlags flags, const vkb::core::render_pass* render_pass, const vkb::core::framebuffer* framebuffer,
                                         uint32_t subpass_index)
        {
            // Reset state
            pipeline_state_.reset();
            resource_binding_state_.reset();
            descriptor_set_layout_binding_state_.clear();
            stored_push_constants_.clear();

            vk::CommandBufferBeginInfo begin_info(flags);
            vk::CommandBufferInheritanceInfo inheritance;

            if (level_ == vk::CommandBufferLevel::eSecondary)
            {
                assert((render_pass && framebuffer) && "Render pass and framebuffer must be provided when calling begin from a secondary one");

                current_render_pass_.render_pass = render_pass;
                current_render_pass_.framebuffer = framebuffer;

                inheritance.renderPass = current_render_pass_.render_pass->get_handle();
                inheritance.framebuffer = current_render_pass_.framebuffer->handle();
                inheritance.subpass = subpass_index;

                begin_info.pInheritanceInfo = &inheritance;
            }

            get_handle().begin(begin_info);
            return vk::Result::eSuccess;
        }

        void command_buffer::begin_query(const vkb::core::query_pool& query_pool, uint32_t query, vk::QueryControlFlags flags)
        {
            get_handle().beginQuery(query_pool.handle(), query, flags);
        }

        void command_buffer::begin_render_pass(const vkb::rendering::render_target& render_target,
                                               const std::vector<vkb::common::load_store_info>& load_store_infos,
                                               const std::vector<vk::ClearValue>& clear_values,
                                               const std::vector<std::unique_ptr<vkb::rendering::subpass>>& subpasses,
                                               vk::SubpassContents contents)
        {
            // Reset state
            pipeline_state_.reset();
            resource_binding_state_.reset();
            descriptor_set_layout_binding_state_.clear();

            auto& render_pass = get_render_pass(render_target, load_store_infos, subpasses);
            auto& framebuffer = get_device().get_resource_cache().request_framebuffer(render_target, render_pass);

            begin_render_pass(render_target, render_pass, framebuffer, clear_values, contents);
        }

        void command_buffer::begin_render_pass(const vkb::rendering::render_target& render_target,
                                               const vkb::core::render_pass& render_pass,
                                               const vkb::core::framebuffer& framebuffer,
                                               const std::vector<vk::ClearValue>& clear_values,
                                               vk::SubpassContents contents)
        {
            current_render_pass_.render_pass = &render_pass;
            current_render_pass_.framebuffer = &framebuffer;

            // Begin render pass
            vk::RenderPassBeginInfo begin_info(
                current_render_pass_.render_pass->get_handle(), current_render_pass_.framebuffer->handle(), {{}, render_target.extent()}, clear_values);

            const auto& framebuffer_extent = current_render_pass_.framebuffer->extent();

            // Test the requested render area to confirm that it is optimal and could not cause a performance reduction
            if (!is_render_size_optimal(framebuffer_extent, begin_info.renderArea))
            {
                // Only prints the warning if the framebuffer or render area are different since the last time the render size was not optimal
                if ((framebuffer_extent != last_framebuffer_extent_) || (begin_info.renderArea.extent != last_render_area_extent_))
                {
                    LOGW("Render target extent is not an optimal size, this may result in reduced performance.");
                }

                last_framebuffer_extent_ = framebuffer_extent;
                last_render_area_extent_ = begin_info.renderArea.extent;
            }

            get_handle().beginRenderPass(begin_info, contents);

            // Update blend state attachments for first subpass
            auto blend_state = pipeline_state_.get_color_blend_state();
            blend_state.attachments.resize(current_render_pass_.render_pass->get_color_output_count(pipeline_state_.get_subpass_index()));
            pipeline_state_.set_color_blend_state(blend_state);
        }

        void command_buffer::bind_buffer(
            const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            resource_binding_state_.bind_buffer(buffer, offset, range, set, binding, array_element);
        }

        void command_buffer::bind_image(
            const vkb::core::image_view& image_view, const vkb::core::sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            resource_binding_state_.bind_image(image_view, sampler, set, binding, array_element);
        }

        void command_buffer::bind_image(const vkb::core::image_view& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            resource_binding_state_.bind_image(image_view, set, binding, array_element);
        }

        void command_buffer::bind_index_buffer(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, vk::IndexType index_type)
        {
            get_handle().bindIndexBuffer(buffer.get_handle(), offset, index_type);
        }

        void command_buffer::bind_input(const vkb::core::image_view& image_view, uint32_t set, uint32_t binding, uint32_t array_element)
        {
            resource_binding_state_.bind_input(image_view, set, binding, array_element);
        }

        void command_buffer::bind_lighting(vkb::rendering::lighting_state& lighting_state, uint32_t set, uint32_t binding)
        {
            bind_buffer(lighting_state.light_buffer.buffer(), lighting_state.light_buffer.offset(), lighting_state.light_buffer.size(), set, binding, 0);

            set_specialization_constant(0, to_u32(lighting_state.directional_lights.size()));
            set_specialization_constant(1, to_u32(lighting_state.point_lights.size()));
            set_specialization_constant(2, to_u32(lighting_state.spot_lights.size()));
        }

        void command_buffer::bind_pipeline_layout(vkb::core::pipeline_layout& pipeline_layout)
        {
            pipeline_state_.set_pipeline_layout(pipeline_layout);
        }

        void command_buffer::bind_vertex_buffers(uint32_t first_binding,
                                                 const std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>>& buffers,
                                                 const std::vector<vk::DeviceSize>& offsets)
        {
            std::vector<vk::Buffer> buffer_handles(buffers.size(), nullptr);
            std::transform(buffers.begin(), buffers.end(), buffer_handles.begin(), [](const vkb::core::HPPBuffer& buffer) { return buffer.get_handle(); });
            get_handle().bindVertexBuffers(first_binding, buffer_handles, offsets);
        }

        void command_buffer::blit_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageBlit>& regions)
        {
            get_handle().blitImage(
                src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions, vk::Filter::eNearest);
        }

        void command_buffer::buffer_memory_barrier(const vkb::core::HPPBuffer& buffer,
                                                   vk::DeviceSize offset,
                                                   vk::DeviceSize size,
                                                   const vkb::common::buffer_memory_barrier& memory_barrier)
        {
            vk::BufferMemoryBarrier buffer_memory_barrier(memory_barrier.src_access_mask, memory_barrier.dst_access_mask, {}, {}, buffer.get_handle(), offset, size);

            vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
            vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

            get_handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, buffer_memory_barrier, {});
        }

        void command_buffer::clear(vk::ClearAttachment attachment, vk::ClearRect rect)
        {
            get_handle().clearAttachments(attachment, rect);
        }

        void command_buffer::copy_buffer(const vkb::core::HPPBuffer& src_buffer, const vkb::core::HPPBuffer& dst_buffer, vk::DeviceSize size)
        {
            vk::BufferCopy copy_region({}, {}, size);
            get_handle().copyBuffer(src_buffer.get_handle(), dst_buffer.get_handle(), copy_region);
        }

        void command_buffer::copy_buffer_to_image(const vkb::core::HPPBuffer& buffer,
                                                  const vkb::core::image& image,
                                                  const std::vector<vk::BufferImageCopy>& regions)
        {
            get_handle().copyBufferToImage(buffer.get_handle(), image.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
        }

        void command_buffer::copy_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageCopy>& regions)
        {
            get_handle().copyImage(src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
        }

        void command_buffer::copy_image_to_buffer(const vkb::core::image& image,
                                                  vk::ImageLayout image_layout,
                                                  const vkb::core::HPPBuffer& buffer,
                                                  const std::vector<vk::BufferImageCopy>& regions)
        {
            get_handle().copyImageToBuffer(image.get_handle(), image_layout, buffer.get_handle(), regions);
        }

        void command_buffer::dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
        {
            flush(vk::PipelineBindPoint::eCompute);
            get_handle().dispatch(group_count_x, group_count_y, group_count_z);
        }

        void command_buffer::dispatch_indirect(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset)
        {
            flush(vk::PipelineBindPoint::eCompute);
            get_handle().dispatchIndirect(buffer.get_handle(), offset);
        }

        void command_buffer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            get_handle().draw(vertex_count, instance_count, first_vertex, first_instance);
        }

        void command_buffer::draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            get_handle().drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
        }

        void command_buffer::draw_indexed_indirect(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride)
        {
            flush(vk::PipelineBindPoint::eGraphics);
            get_handle().drawIndexedIndirect(buffer.get_handle(), offset, draw_count, stride);
        }

        vk::Result command_buffer::end()
        {
            get_handle().end();

            return vk::Result::eSuccess;
        }

        void command_buffer::end_query(const vkb::core::query_pool& query_pool, uint32_t query)
        {
            get_handle().endQuery(query_pool.handle(), query);
        }

        void command_buffer::end_render_pass()
        {
            get_handle().endRenderPass();
        }

        void command_buffer::execute_commands(command_buffer& secondary_command_buffer)
        {
            get_handle().executeCommands(secondary_command_buffer.get_handle());
        }

        void command_buffer::execute_commands(std::vector<command_buffer*>& secondary_command_buffers)
        {
            std::vector<vk::CommandBuffer> sec_cmd_buf_handles(secondary_command_buffers.size(), nullptr);
            std::transform(secondary_command_buffers.begin(),
                           secondary_command_buffers.end(),
                           sec_cmd_buf_handles.begin(),
                           [](const vkb::core::command_buffer* sec_cmd_buf) { return sec_cmd_buf->get_handle(); });
            get_handle().executeCommands(sec_cmd_buf_handles);
        }

        vkb::core::render_pass& command_buffer::get_render_pass(const vkb::rendering::render_target& render_target,
                                                                const std::vector<vkb::common::load_store_info>& load_store_infos,
                                                                const std::vector<std::unique_ptr<vkb::rendering::subpass>>& subpasses)
        {
            // Create render pass
            assert(subpasses.size() > 0 && "Cannot create a render pass without any subpass");

            std::vector<vkb::core::subpass_info> subpass_infos(subpasses.size());
            auto subpass_info_it = subpass_infos.begin();
            for (auto& subpass : subpasses)
            {
                subpass_info_it->input_attachments = subpass->get_input_attachments();
                subpass_info_it->output_attachments = subpass->get_output_attachments();
                subpass_info_it->color_resolve_attachments = subpass->get_color_resolve_attachments();
                subpass_info_it->disable_depth_stencil_attachment = subpass->get_disable_depth_stencil_attachment();
                subpass_info_it->depth_stencil_resolve_mode = subpass->depth_stencil_resolve_mode();
                subpass_info_it->depth_stencil_resolve_attachment = subpass->get_depth_stencil_resolve_attachment();
                subpass_info_it->debug_name = subpass->get_debug_name();

                ++subpass_info_it;
            }

            return get_device().get_resource_cache().request_render_pass(render_target.attachments(), load_store_infos, subpass_infos);
        }

        void command_buffer::image_memory_barrier(const vkb::core::image_view& image_view, const vkb::common::image_memory_barrier& memory_barrier) const
        {
            // Adjust barrier's subresource range for depth images
            auto subresource_range = image_view.subresource_range();
            auto format = image_view.format();
            if (vkb::common::is_depth_only_format(format))
            {
                subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
            }
            else if (vkb::common::is_depth_stencil_format(format))
            {
                subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
            }

            vk::ImageMemoryBarrier image_memory_barrier(memory_barrier.src_access_mask,
                                                        memory_barrier.dst_access_mask,
                                                        memory_barrier.old_layout,
                                                        memory_barrier.new_layout,
                                                        memory_barrier.old_queue_family,
                                                        memory_barrier.new_queue_family,
                                                        image_view.image().get_handle(),
                                                        subresource_range);

            vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
            vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

            get_handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
        }

        void command_buffer::next_subpass()
        {
            // Increment subpass index
            pipeline_state_.set_subpass_index(pipeline_state_.get_subpass_index() + 1);

            // Update blend state attachments
            auto blend_state = pipeline_state_.get_color_blend_state();
            blend_state.attachments.resize(current_render_pass_.render_pass->get_color_output_count(pipeline_state_.get_subpass_index()));
            pipeline_state_.set_color_blend_state(blend_state);

            // Reset descriptor sets
            resource_binding_state_.reset();
            descriptor_set_layout_binding_state_.clear();

            // Clear stored push constants
            stored_push_constants_.clear();

            get_handle().nextSubpass(vk::SubpassContents::eInline);
        }

        void command_buffer::push_constants(const std::vector<uint8_t>& values)
        {
            uint32_t push_constant_size = to_u32(stored_push_constants_.size() + values.size());

            if (push_constant_size > max_push_constants_size_)
            {
                LOGE("Push constant limit of {} exceeded (pushing {} bytes for a total of {} bytes)", max_push_constants_size_, values.size(), push_constant_size);
                throw std::runtime_error("Push constant limit exceeded.");
            }
            else
            {
                stored_push_constants_.insert(stored_push_constants_.end(), values.begin(), values.end());
            }
        }

        vk::Result command_buffer::reset(reset_mode reset_mode)
        {
            assert(reset_mode == command_pool_.get_reset_mode() && "Command buffer reset mode must match the one used by the pool to allocate it");

            if (reset_mode == reset_mode::ResetIndividually)
            {
                get_handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
            }

            return vk::Result::eSuccess;
        }

        void command_buffer::reset_query_pool(const vkb::core::query_pool& query_pool, uint32_t first_query, uint32_t query_count)
        {
            get_handle().resetQueryPool(query_pool.handle(), first_query, query_count);
        }

        void command_buffer::resolve_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageResolve>& regions)
        {
            get_handle().resolveImage(src_img.get_handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.get_handle(), vk::ImageLayout::eTransferDstOptimal, regions);
        }

        void command_buffer::set_blend_constants(const std::array<float, 4>& blend_constants)
        {
            get_handle().setBlendConstants(blend_constants.data());
        }

        void command_buffer::set_color_blend_state(const vkb::rendering::color_blend_state& state_info)
        {
            pipeline_state_.set_color_blend_state(state_info);
        }

        void command_buffer::set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor)
        {
            get_handle().setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
        }

        void command_buffer::set_depth_bounds(float min_depth_bounds, float max_depth_bounds)
        {
            get_handle().setDepthBounds(min_depth_bounds, max_depth_bounds);
        }

        void command_buffer::set_depth_stencil_state(const vkb::rendering::depth_stencil_state& state_info)
        {
            pipeline_state_.set_depth_stencil_state(state_info);
        }

        void command_buffer::set_input_assembly_state(const vkb::rendering::input_assembly_state& state_info)
        {
            pipeline_state_.set_input_assembly_state(state_info);
        }

        void command_buffer::set_line_width(float line_width)
        {
            get_handle().setLineWidth(line_width);
        }

        void command_buffer::set_multisample_state(const vkb::rendering::multisample_state& state_info)
        {
            pipeline_state_.set_multisample_state(state_info);
        }

        void command_buffer::set_rasterization_state(const vkb::rendering::rasterization_state& state_info)
        {
            pipeline_state_.set_rasterization_state(state_info);
        }

        void command_buffer::set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors)
        {
            get_handle().setScissor(first_scissor, scissors);
        }

        void command_buffer::set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data)
        {
            pipeline_state_.set_specialization_constant(constant_id, data);
        }

        void command_buffer::set_update_after_bind(bool update_after_bind_)
        {
            update_after_bind_ = update_after_bind_;
        }

        void command_buffer::set_vertex_input_state(const vkb::rendering::vertex_input_state& state_info)
        {
            pipeline_state_.set_vertex_input_state(state_info);
        }

        void command_buffer::set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport>& viewports)
        {
            get_handle().setViewport(first_viewport, viewports);
        }

        void command_buffer::set_viewport_state(const vkb::rendering::viewport_state& state_info)
        {
            pipeline_state_.set_viewport_state(state_info);
        }

        void command_buffer::update_buffer(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data)
        {
            get_handle().updateBuffer<uint8_t>(buffer.get_handle(), offset, data);
        }

        void command_buffer::write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const vkb::core::query_pool& query_pool, uint32_t query)
        {
            get_handle().writeTimestamp(pipeline_stage, query_pool.handle(), query);
        }

        void command_buffer::flush(vk::PipelineBindPoint pipeline_bind_point)
        {
            flush_pipeline_state(pipeline_bind_point);
            flush_push_constants();
            flush_descriptor_state(pipeline_bind_point);
        }

        void command_buffer::flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point)
        {
            assert(command_pool_.get_render_frame() && "The command pool must be associated to a render frame");

            const auto& pipeline_layout = pipeline_state_.get_pipeline_layout();

            std::unordered_set<uint32_t> update_descriptor_sets;

            // Iterate over the shader sets to check if they have already been bound
            // If they have, add the set so that the command buffer later updates it
            for (auto& set_it : pipeline_layout.shader_sets())
            {
                uint32_t descriptor_set_id = set_it.first;

                auto descriptor_set_layout_it = descriptor_set_layout_binding_state_.find(descriptor_set_id);

                if (descriptor_set_layout_it != descriptor_set_layout_binding_state_.end())
                {
                    if (descriptor_set_layout_it->second->get_handle() != pipeline_layout.descriptor_set_layout(descriptor_set_id).get_handle())
                    {
                        update_descriptor_sets.emplace(descriptor_set_id);
                    }
                }
            }

            // Validate that the bound descriptor set layouts exist in the pipeline layout
            for (auto set_it = descriptor_set_layout_binding_state_.begin(); set_it != descriptor_set_layout_binding_state_.end();)
            {
                if (!pipeline_layout.has_descriptor_set_layout(set_it->first))
                {
                    set_it = descriptor_set_layout_binding_state_.erase(set_it);
                }
                else
                {
                    ++set_it;
                }
            }

            // Check if a descriptor set needs to be created
            if (resource_binding_state_.is_dirty() || !update_descriptor_sets.empty())
            {
                resource_binding_state_.clear_dirty();

                // Iterate over all of the resource sets bound by the command buffer
                for (auto& resource_set_it : resource_binding_state_.get_resource_sets())
                {
                    uint32_t descriptor_set_id = resource_set_it.first;
                    auto& resource_set = resource_set_it.second;

                    // Don't update resource set if it's not in the update list OR its state hasn't changed
                    if (!resource_set.is_dirty() && (update_descriptor_sets.find(descriptor_set_id) == update_descriptor_sets.end()))
                    {
                        continue;
                    }

                    // Clear dirty flag for resource set
                    resource_binding_state_.clear_dirty(descriptor_set_id);

                    // Skip resource set if a descriptor set layout doesn't exist for it
                    if (!pipeline_layout.has_descriptor_set_layout(descriptor_set_id))
                    {
                        continue;
                    }

                    auto& descriptor_set_layout = pipeline_layout.descriptor_set_layout(descriptor_set_id);

                    // Make descriptor set layout bound for current set
                    descriptor_set_layout_binding_state_[descriptor_set_id] = &descriptor_set_layout;

                    BindingMap<vk::DescriptorBufferInfo> buffer_infos;
                    BindingMap<vk::DescriptorImageInfo> image_infos;

                    std::vector<uint32_t> dynamic_offsets;

                    // Iterate over all resource bindings
                    for (auto& binding_it : resource_set.get_resource_bindings())
                    {
                        auto binding_index = binding_it.first;
                        auto& binding_resources = binding_it.second;

                        // Check if binding exists in the pipeline layout
                        if (auto binding_info = descriptor_set_layout.get_layout_binding(binding_index))
                        {
                            // Iterate over all binding resources
                            for (auto& element_it : binding_resources)
                            {
                                auto array_element = element_it.first;
                                auto& resource_info = element_it.second;

                                // Pointer references
                                auto& buffer = resource_info.buffer;
                                auto& sampler = resource_info.sampler;
                                auto& image_view = resource_info.image_view;

                                // Get buffer info
                                if (buffer != nullptr && vkb::common::is_buffer_descriptor_type(binding_info->descriptorType))
                                {
                                    vk::DescriptorBufferInfo buffer_info(resource_info.buffer->get_handle(), resource_info.offset, resource_info.range);

                                    if (vkb::common::is_dynamic_buffer_descriptor_type(binding_info->descriptorType))
                                    {
                                        dynamic_offsets.push_back(to_u32(buffer_info.offset));
                                        buffer_info.offset = 0;
                                    }

                                    buffer_infos[binding_index][array_element] = buffer_info;
                                }

                                // Get image info
                                else if (image_view != nullptr || sampler != nullptr)
                                {
                                    // Can be null for input attachments
                                    vk::DescriptorImageInfo image_info(sampler ? sampler->get_handle() : nullptr, image_view->get_handle());

                                    if (image_view != nullptr)
                                    {
                                        // Add image layout info based on descriptor type
                                        switch (binding_info->descriptorType)
                                        {
                                        case vk::DescriptorType::eCombinedImageSampler:
                                            image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                                            break;
                                        case vk::DescriptorType::eInputAttachment:
                                            image_info.imageLayout =
                                                vkb::common::is_depth_format(image_view->format())
                                                    ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                                    : vk::ImageLayout::eShaderReadOnlyOptimal;
                                            break;
                                        case vk::DescriptorType::eStorageImage:
                                            image_info.imageLayout = vk::ImageLayout::eGeneral;
                                            break;
                                        default:
                                            continue;
                                        }
                                    }

                                    image_infos[binding_index][array_element] = image_info;
                                }
                            }

                            assert((!update_after_bind_ ||
                                    (buffer_infos.count(binding_index) > 0 || (image_infos.count(binding_index) > 0))) &&
                                "binding index with no buffer or image infos can't be checked for adding to bindings_to_update");
                        }
                    }

                    vk::DescriptorSet descriptor_set_handle = command_pool_.get_render_frame()->request_descriptor_set(
                        descriptor_set_layout, buffer_infos, image_infos, update_after_bind_, command_pool_.get_thread_index());

                    // Bind descriptor set
                    get_handle().bindDescriptorSets(pipeline_bind_point, pipeline_layout.handle(), descriptor_set_id, descriptor_set_handle, dynamic_offsets);
                }
            }
        }

        void command_buffer::flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point)
        {
            // Create a new pipeline only if the graphics state changed
            if (!pipeline_state_.is_dirty())
            {
                return;
            }

            pipeline_state_.clear_dirty();

            // Create and bind pipeline
            if (pipeline_bind_point == vk::PipelineBindPoint::eGraphics)
            {
                pipeline_state_.set_render_pass(*current_render_pass_.render_pass);
                auto& pipeline = get_device().get_resource_cache().request_graphics_pipeline(pipeline_state_);

                get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
            }
            else if (pipeline_bind_point == vk::PipelineBindPoint::eCompute)
            {
                auto& pipeline = get_device().get_resource_cache().request_compute_pipeline(pipeline_state_);

                get_handle().bindPipeline(pipeline_bind_point, pipeline.get_handle());
            }
            else
            {
                throw "Only graphics and compute pipeline bind points are supported now";
            }
        }

        void command_buffer::flush_push_constants()
        {
            if (stored_push_constants_.empty())
            {
                return;
            }

            const vkb::core::pipeline_layout& pipeline_layout = pipeline_state_.get_pipeline_layout();

            vk::ShaderStageFlags shader_stage = pipeline_layout.push_constant_range_stage(to_u32(stored_push_constants_.size()));

            if (shader_stage)
            {
                get_handle().pushConstants<uint8_t>(pipeline_layout.handle(), shader_stage, 0, stored_push_constants_);
            }
            else
            {
                LOGW("Push constant range [{}, {}] not found", 0, stored_push_constants_.size());
            }

            stored_push_constants_.clear();
        }

        const command_buffer::render_pass_binding& command_buffer::current_render_pass() const
        {
            return current_render_pass_;
        }

        const uint32_t command_buffer::current_subpass_index() const
        {
            return pipeline_state_.get_subpass_index();
        }

        const bool command_buffer::is_render_size_optimal(const vk::Extent2D& framebuffer_extent, const vk::Rect2D& render_area)
        {
            auto render_area_granularity = current_render_pass_.render_pass->get_render_area_granularity();

            return ((render_area.offset.x % render_area_granularity.width == 0) && (render_area.offset.y % render_area_granularity.height == 0) &&
                ((render_area.extent.width % render_area_granularity.width == 0) || (render_area.offset.x + render_area.extent.width == framebuffer_extent.width)) &&
                ((render_area.extent.height % render_area_granularity.height == 0) || (render_area.offset.y + render_area.extent.height == framebuffer_extent.height)));
        }
    } // namespace core
} // namespace vkb
