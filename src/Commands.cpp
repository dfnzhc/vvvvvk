/**
 * @File Commands.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/23
 * @Brief 
 */

#include "Commands.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "ImageView.hpp"

void
CommandPool::init(vk::Device device, uint32_t familyIndex, vk::CommandPoolCreateFlags flags, vk::Queue defaultQueue)
{
    assert(!_device);
    _device = device;

    vk::CommandPoolCreateInfo info;
    info.flags            = flags;
    info.queueFamilyIndex = familyIndex;
    VK_CHECK(_device.createCommandPool(&info, nullptr, &_commandPool));

    if (defaultQueue) {
        _queue = defaultQueue;
    } else {
        _device.getQueue(familyIndex, 0, &_queue);
    }
}

void CommandPool::deinit()
{
    if (_commandPool) {
        _device.destroyCommandPool(_commandPool);
        _commandPool = VK_NULL_HANDLE;
    }
}

vk::CommandBuffer CommandPool::createCommandBuffer(vk::CommandBufferLevel level /*= VK_COMMAND_BUFFER_LEVEL_PRIMARY*/,
                                                   bool begin,
                                                   vk::CommandBufferUsageFlags flags /*= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT*/,
                                                   const vk::CommandBufferInheritanceInfo* pInheritanceInfo /*= nullptr*/)
{
    vk::CommandBufferAllocateInfo allocInfo;
    allocInfo.level              = level;
    allocInfo.commandPool        = _commandPool;
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer cmd;
    VK_CHECK(_device.allocateCommandBuffers(&allocInfo, &cmd));

    if (begin) {
        vk::CommandBufferBeginInfo beginInfo{};
        beginInfo.flags            = flags;
        beginInfo.pInheritanceInfo = pInheritanceInfo;

        cmd.begin(beginInfo);
    }

    return cmd;
}

void CommandPool::free(size_t count, const vk::CommandBuffer* cmds)
{
    _device.freeCommandBuffers(_commandPool, (uint32_t) count, cmds);
}

void CommandPool::submitAndWait(size_t count, const vk::CommandBuffer* cmds, vk::Queue queue)
{
    submit(count, cmds, queue);
    VK_CHECK(vkQueueWaitIdle(queue));
    _device.freeCommandBuffers(_commandPool, (uint32_t) count, cmds);
}

void CommandPool::submit(size_t count, const vk::CommandBuffer* cmds, vk::Queue queue, vk::Fence fence)
{
    for (size_t i = 0; i < count; i++) {
        cmds[i].end();
    }

    vk::SubmitInfo submit;
    submit.pCommandBuffers    = cmds;
    submit.commandBufferCount = (uint32_t) count;

    VK_CHECK(queue.submit(1, &submit, fence));
}

void CommandPool::submit(size_t count, const vk::CommandBuffer* cmds, vk::Fence fence)
{
    submit(count, cmds, _queue, fence);
}

void CommandPool::submit(const std::vector<vk::CommandBuffer>& cmds, vk::Fence fence)
{
    submit(cmds.size(), cmds.data(), _queue, fence);
}

void
copy_buffer(vk::CommandBuffer cmd_buf, const vk_buffer& src_buffer, const vk_buffer& dst_buffer, vk::DeviceSize size)
{
    vk::BufferCopy copy_region({}, {}, size);
    if (size == 0)
    {
        copy_region.size = src_buffer.get_size();
    }
    
    cmd_buf.copyBuffer(src_buffer.handle(), dst_buffer.handle(), copy_region);
}

void copy_image(vk::CommandBuffer cmd_buf, const vk_image& src_img, const vk_image& dst_img,
                const std::vector<vk::ImageCopy>& regions)
{
    cmd_buf.copyImage(src_img.handle(), vk::ImageLayout::eTransferSrcOptimal, dst_img.handle(),
                      vk::ImageLayout::eTransferDstOptimal, regions);
}

void copy_buffer_to_image(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, const vk_image& image,
                          const std::vector<vk::BufferImageCopy>& regions)
{

    cmd_buf.copyBufferToImage(buffer.handle(), image.handle(), vk::ImageLayout::eTransferDstOptimal, regions);
}

void copy_image_to_buffer(vk::CommandBuffer cmd_buf, const vk_image& image, vk::ImageLayout image_layout,
                          const vk_buffer& buffer, const std::vector<vk::BufferImageCopy>& regions)
{
    cmd_buf.copyImageToBuffer(image.handle(), image_layout, buffer.handle(), regions);
}

void
image_memory_barrier(vk::CommandBuffer cmd_buf, const vk_image_view& image_view,
                     const ImageMemoryBarrier& memory_barrier)
{
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

    cmd_buf.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, {}, image_memory_barrier);
}

void set_viewport(vk::CommandBuffer cmd_buf, uint32_t first_viewport, const std::vector<vk::Viewport>& viewports)
{
    cmd_buf.setViewport(first_viewport, viewports);
}

void set_scissor(vk::CommandBuffer cmd_buf, uint32_t first_scissor, const std::vector<vk::Rect2D>& scissors)
{

    cmd_buf.setScissor(first_scissor, scissors);
}

void set_line_width(vk::CommandBuffer cmd_buf, float line_width)
{

    cmd_buf.setLineWidth(line_width);
}

void set_depth_bias(vk::CommandBuffer cmd_buf, float depth_bias_constant_factor, float depth_bias_clamp,
                    float depth_bias_slope_factor)
{

    cmd_buf.setDepthBias(depth_bias_constant_factor, depth_bias_clamp, depth_bias_slope_factor);
}

void set_blend_constants(vk::CommandBuffer cmd_buf, const std::array<float, 4>& blend_constants)
{
    cmd_buf.setBlendConstants(blend_constants.data());

}

void set_depth_bounds(vk::CommandBuffer cmd_buf, float min_depth_bounds, float max_depth_bounds)
{
    cmd_buf.setDepthBounds(min_depth_bounds, max_depth_bounds);
}

void update_buffer(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, vk::DeviceSize offset,
                   const std::vector<uint8_t>& data)
{
    cmd_buf.updateBuffer<uint8_t>(buffer.handle(), offset, data);
}

void
buffer_memory_barrier(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size,
                      const BufferMemoryBarrier& memory_barrier)
{
    vk::BufferMemoryBarrier buffer_memory_barrier(memory_barrier.src_access_mask, memory_barrier.dst_access_mask, {},
                                                  {}, buffer.handle(), offset, size);

    vk::PipelineStageFlags src_stage_mask = memory_barrier.src_stage_mask;
    vk::PipelineStageFlags dst_stage_mask = memory_barrier.dst_stage_mask;

    cmd_buf.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, {}, buffer_memory_barrier, {});
}
