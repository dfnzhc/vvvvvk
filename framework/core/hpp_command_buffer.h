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

#include <common/hpp_vk_common.h>
#include <core/hpp_framebuffer.h>
#include <core/hpp_query_pool.h>
#include <hpp_resource_binding_state.h>
#include <rendering/hpp_pipeline_state.h>
#include <rendering/hpp_render_target.h>
#include <rendering/hpp_subpass.h>

namespace vkb
{
    namespace core
    {
        class command_pool;
        class descriptor_set_layout;

        /**
         * @brief Helper class to manage and record a command buffer, building and
         *        keeping track of pipeline state and resource bindings
         */
        class command_buffer : public core::vk_unit<vk::CommandBuffer>
        {
        public:
            struct render_pass_binding
            {
                const vkb::core::render_pass* render_pass;
                const vkb::core::framebuffer* framebuffer;
            };

            enum class reset_mode
            {
                ResetPool,
                ResetIndividually,
                AlwaysAllocate,
            };

        public:
            command_buffer(vkb::core::command_pool& command_pool, vk::CommandBufferLevel level);
            command_buffer(command_buffer&& other);
            ~command_buffer();

            command_buffer(const command_buffer&) = delete;
            command_buffer& operator=(const command_buffer&) = delete;
            command_buffer& operator=(command_buffer&&) = delete;

            vk::Result begin(vk::CommandBufferUsageFlags flags, command_buffer* primary_cmd_buf = nullptr);
            vk::Result begin(vk::CommandBufferUsageFlags flags, const vkb::core::render_pass* render_pass, const vkb::core::framebuffer* framebuffer, uint32_t subpass_index);

            void begin_query(const vkb::core::query_pool& query_pool, uint32_t query, vk::QueryControlFlags flags);
            void begin_render_pass(const vkb::rendering::render_target& render_target,
                                   const std::vector<vkb::common::load_store_info>& load_store_infos,
                                   const std::vector<vk::ClearValue>& clear_values,
                                   const std::vector<std::unique_ptr<vkb::rendering::subpass>>& subpasses,
                                   vk::SubpassContents contents = vk::SubpassContents::eInline);
            void begin_render_pass(const vkb::rendering::render_target& render_target,
                                   const vkb::core::render_pass& render_pass,
                                   const vkb::core::framebuffer& framebuffer,
                                   const std::vector<vk::ClearValue>& clear_values,
                                   vk::SubpassContents contents = vk::SubpassContents::eInline);
            void bind_buffer(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, vk::DeviceSize range, uint32_t set, uint32_t binding, uint32_t array_element);
            void bind_image(const vkb::core::image_view& image_view, const vkb::core::sampler& sampler, uint32_t set, uint32_t binding, uint32_t array_element);
            void bind_image(const vkb::core::image_view& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
            void bind_index_buffer(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, vk::IndexType index_type);
            void bind_input(const vkb::core::image_view& image_view, uint32_t set, uint32_t binding, uint32_t array_element);
            void bind_lighting(vkb::rendering::lighting_state& lighting_state, uint32_t set, uint32_t binding);
            void bind_pipeline_layout(vkb::core::pipeline_layout& pipeline_layout);
            void bind_vertex_buffers(uint32_t first_binding,
                                     const std::vector<std::reference_wrapper<const vkb::core::HPPBuffer>>& buffers,
                                     const std::vector<vk::DeviceSize>& offsets);
            void blit_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageBlit>& regions);
            void buffer_memory_barrier(const vkb::core::HPPBuffer& buffer,
                                       vk::DeviceSize offset,
                                       vk::DeviceSize size,
                                       const vkb::common::buffer_memory_barrier& memory_barrier);
            void clear(vk::ClearAttachment info, vk::ClearRect rect);
            void copy_buffer(const vkb::core::HPPBuffer& src_buffer, const vkb::core::HPPBuffer& dst_buffer, vk::DeviceSize size);
            void copy_buffer_to_image(const vkb::core::HPPBuffer& buffer, const vkb::core::image& image, const std::vector<vk::BufferImageCopy>& regions);
            void copy_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageCopy>& regions);
            void copy_image_to_buffer(const vkb::core::image& image,
                                      vk::ImageLayout image_layout,
                                      const vkb::core::HPPBuffer& buffer,
                                      const std::vector<vk::BufferImageCopy>& regions);
            void dispatch(uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
            void dispatch_indirect(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset);
            void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
            void draw_indexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, int32_t vertex_offset, uint32_t first_instance);
            void draw_indexed_indirect(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, uint32_t draw_count, uint32_t stride);
            vk::Result end();
            void end_query(const vkb::core::query_pool& query_pool, uint32_t query);
            void end_render_pass();
            void execute_commands(command_buffer& secondary_command_buffer);
            void execute_commands(std::vector<command_buffer*>& secondary_command_buffers);
            vkb::core::render_pass& get_render_pass(const vkb::rendering::render_target& render_target,
                                                    const std::vector<vkb::common::load_store_info>& load_store_infos,
                                                    const std::vector<std::unique_ptr<vkb::rendering::subpass>>& subpasses);
            void image_memory_barrier(const vkb::core::image_view& image_view, const vkb::common::image_memory_barrier& memory_barrier) const;
            void next_subpass();

            void push_constants(const std::vector<uint8_t>& values);

            template <typename T>
            void push_constants(const T& value)
            {
                auto data = to_bytes(value);

                uint32_t size = to_u32(stored_push_constants_.size() + data.size());

                if (size > max_push_constants_size_)
                {
                    LOGE("Push constant limit exceeded ({} / {} bytes)", size, max_push_constants_size_);
                    throw std::runtime_error("Cannot overflow push constant limit");
                }

                stored_push_constants_.insert(stored_push_constants_.end(), data.begin(), data.end());
            }

            vk::Result reset(reset_mode reset_mode);

            void reset_query_pool(const vkb::core::query_pool& query_pool, uint32_t first_query, uint32_t query_count);
            void resolve_image(const vkb::core::image& src_img, const vkb::core::image& dst_img, const std::vector<vk::ImageResolve>& regions);
            void set_blend_constants(const std::array<float, 4>& blend_constants);
            void set_color_blend_state(const vkb::rendering::color_blend_state& state_info);
            void set_depth_bias(float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
            void set_depth_bounds(float min_depth_bounds, float max_depth_bounds);
            void set_depth_stencil_state(const vkb::rendering::depth_stencil_state& state_info);
            void set_input_assembly_state(const vkb::rendering::input_assembly_state& state_info);
            void set_line_width(float line_width);
            void set_multisample_state(const vkb::rendering::multisample_state& state_info);
            void set_rasterization_state(const vkb::rendering::rasterization_state& state_info);
            void set_scissor(uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors);

            template <class T>
            void set_specialization_constant(uint32_t constant_id, const T& data);
            void set_specialization_constant(uint32_t constant_id, const std::vector<uint8_t>& data);

            void set_update_after_bind(bool update_after_bind_);
            void set_vertex_input_state(const vkb::rendering::vertex_input_state& state_info);
            void set_viewport(uint32_t first_viewport, const std::vector<vk::Viewport>& viewports);
            void set_viewport_state(const vkb::rendering::viewport_state& state_info);
            void update_buffer(const vkb::core::HPPBuffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data);
            void write_timestamp(vk::PipelineStageFlagBits pipeline_stage, const vkb::core::query_pool& query_pool, uint32_t query);

        private:
            void flush(vk::PipelineBindPoint pipeline_bind_point);

            void flush_descriptor_state(vk::PipelineBindPoint pipeline_bind_point);
            void flush_pipeline_state(vk::PipelineBindPoint pipeline_bind_point);
            void flush_push_constants();

            const render_pass_binding& current_render_pass() const;
            const uint32_t current_subpass_index() const;

            const bool is_render_size_optimal(const vk::Extent2D& extent, const vk::Rect2D& render_area);

        private:
            const vk::CommandBufferLevel level_ = {};
            vkb::core::command_pool& command_pool_;
            render_pass_binding current_render_pass_ = {};
            vkb::rendering::pipeline_state pipeline_state_ = {};
            vkb::resource_binding_state resource_binding_state_ = {};

            std::vector<uint8_t> stored_push_constants_ = {};
            uint32_t max_push_constants_size_ = {};

            vk::Extent2D last_framebuffer_extent_ = {};
            vk::Extent2D last_render_area_extent_ = {};

            bool update_after_bind_ = false;

            std::unordered_map<uint32_t, vkb::core::descriptor_set_layout const*> descriptor_set_layout_binding_state_;
        };

        template <class T>
        inline void command_buffer::set_specialization_constant(uint32_t constant_id, const T& data)
        {
            set_specialization_constant(constant_id, to_bytes(data));
        }

        template <>
        inline void command_buffer::set_specialization_constant<bool>(std::uint32_t constant_id, const bool& data)
        {
            set_specialization_constant(constant_id, to_bytes(to_u32(data)));
        }
    } // namespace core
} // namespace vkb
