/**
 * @File CommandBuffer.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "CommandBuffer.hpp"
#include "Debug.hpp"
#include "Device.hpp"
#include "CommandBufferPool.hpp"

vk_command_buffer::vk_command_buffer(vk_command_pool& command_pool, vk::CommandBufferLevel level)
    : vk_unit{nullptr, &command_pool.get_device()},
      level(level),
      command_pool(command_pool)
{
    vk::CommandBufferAllocateInfo allocate_info(command_pool.get_handle(), level, 1);
    set_handle(device().handle().allocateCommandBuffers(allocate_info).front());
}

vk_command_buffer::vk_command_buffer(vk_command_buffer&& other) noexcept
    : vk_unit{std::move(other)},
      level(other.level),
      command_pool(other.command_pool)
{

}

vk_command_buffer::~vk_command_buffer()
{
    if (handle()) {
        device().handle().freeCommandBuffers(command_pool.get_handle(), handle());
    }
}

vk::Result vk_command_buffer::begin(vk::CommandBufferUsageFlags flags, vk_command_buffer* primary_cmd_buf)
{
//    if (level == vk::CommandBufferLevel::eSecondary) {
//        assert(primary_cmd_buf &&
//                   "A primary command buffer pointer must be provided when calling begin from a secondary one");
//        auto const& render_pass_binding = primary_cmd_buf->get_current_render_pass();
//
//        return begin(flags, render_pass_binding.render_pass, render_pass_binding.framebuffer,
//                     primary_cmd_buf->get_current_subpass_index());
//    }

    return begin(flags, nullptr, nullptr, 0);
}

vk::Result vk_command_buffer::begin(vk::CommandBufferUsageFlags flags, const vk_renderpass* render_pass,
                                    const vk_framebuffer* framebuffer, uint32_t subpass_index)
{
//    // Reset state
//    pipeline_state.reset();
//    resource_binding_state.reset();
//    descriptor_set_layout_binding_state.clear();
//    stored_push_constants.clear();

    vk::CommandBufferBeginInfo       begin_info(flags);
//    vk::CommandBufferInheritanceInfo inheritance;

//    if (level == vk::CommandBufferLevel::eSecondary) {
//        assert((render_pass && framebuffer) &&
//               "Render pass and framebuffer must be provided when calling begin from a secondary one");
//
//        current_render_pass.render_pass = render_pass;
//        current_render_pass.framebuffer = framebuffer;
//
//        inheritance.renderPass  = current_render_pass.render_pass->get_handle();
//        inheritance.framebuffer = current_render_pass.framebuffer->get_handle();
//        inheritance.subpass     = subpass_index;
//
//        begin_info.pInheritanceInfo = &inheritance;
//    }

    handle().begin(begin_info);
    return vk::Result::eSuccess;
}

vk::Result vk_command_buffer::end()
{
    handle().end();

    return vk::Result::eSuccess;
}

vk::Result vk_command_buffer::reset(vk_command_buffer::reset_mode reset_mode)
{
    assert(reset_mode == command_pool.get_reset_mode() &&
           "命令缓冲区的重置模式必须与分配它时的池一致");

    if (reset_mode == reset_mode::ResetIndividually) {
        handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    }

    return vk::Result::eSuccess;
}

void vk_command_buffer::copy_buffer(const vk_buffer& src_buffer, const vk_buffer& dst_buffer, vk::DeviceSize size)
{
    vk::BufferCopy copy_region({}, {}, size);
    handle().copyBuffer(src_buffer.handle(), dst_buffer.handle(), copy_region);
}

void vk_command_buffer::copy_image(const vk_image& src_img, const vk_image& dst_img,
                                   const std::vector<vk::ImageCopy>& regions)
{
    handle().copyImage(src_img.handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.handle(),
                       vk::ImageLayout::eTransferDstOptimal, regions);
}

void vk_command_buffer::copy_buffer_to_image(const vk_buffer& buffer, const vk_image& image,
                                             const std::vector<vk::BufferImageCopy>& regions)
{

    handle().copyBufferToImage(buffer.handle(), image.handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}


void vk_command_buffer::copy_image_to_buffer(const vk_image& image, vk::ImageLayout image_layout,
                                             const vk_buffer& buffer, const std::vector<vk::BufferImageCopy>& regions)
{
    handle().copyImageToBuffer(image.handle(), image_layout, buffer.handle(), regions);
}

void
vk_command_buffer::image_memory_barrier(const vk_image_view& image_view, const ImageMemoryBarrier& memory_barrier) const
{
    // Adjust barrier's subresource range for depth images
    auto subresource_range = image_view.get_subresource_range();
    auto format            = image_view.get_format();

    if (is_depth_only_format(format)) {
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth;
    } else if (is_depth_stencil_format(format)) {
        subresource_range.aspectMask = vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    }

    vk::ImageMemoryBarrier image_memory_barrier(memory_barrier.src_access_mask,
                                                memory_barrier.dst_access_mask,
                                                memory_barrier.old_layout,
                                                memory_barrier.new_layout,
                                                memory_barrier.old_queue_family,
                                                memory_barrier.new_queue_family,
                                                image_view.get_image().handle(),
                                                subresource_range);

    vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
    vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

    handle().pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
}



