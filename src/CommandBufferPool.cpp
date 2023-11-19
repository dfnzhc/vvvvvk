/**
 * @File CommandBufferPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "CommandBufferPool.hpp"
#include "Device.hpp"
#include "RenderFrame.hpp"

vk_command_pool::vk_command_pool(vk_device& d,
                                 uint32_t queue_family_index,
                                 vk_render_frame* render_frame,
                                 size_t thread_index,
                                 vk_command_buffer::reset_mode reset_mode) :
    device{d}, render_frame{render_frame}, thread_index{thread_index}, reset_mode{reset_mode}
{
    vk::CommandPoolCreateFlags flags;
    switch (reset_mode) {
        case vk_command_buffer::reset_mode::ResetIndividually:
        case vk_command_buffer::reset_mode::AlwaysAllocate:
            flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            break;
        case vk_command_buffer::reset_mode::ResetPool:
        default:
            flags = vk::CommandPoolCreateFlagBits::eTransient;
            break;
    }

    vk::CommandPoolCreateInfo command_pool_create_info(flags, queue_family_index);
    handle = device.handle().createCommandPool(command_pool_create_info);
}

vk_command_pool::vk_command_pool(vk_command_pool&& other) noexcept :
    device(other.device),
    handle(std::exchange(other.handle, {})),
    queue_family_index(std::exchange(other.queue_family_index, {})),
    primary_command_buffers{std::move(other.primary_command_buffers)},
    active_primary_command_buffer_count(std::exchange(other.active_primary_command_buffer_count, {})),
    secondary_command_buffers{std::move(other.secondary_command_buffers)},
    active_secondary_command_buffer_count(std::exchange(other.active_secondary_command_buffer_count, {})),
    render_frame(std::exchange(other.render_frame, {})),
    thread_index(std::exchange(other.thread_index, {})),
    reset_mode(std::exchange(other.reset_mode, {}))
{
}

vk_command_pool::~vk_command_pool()
{
    // 清楚所有记录的命令缓冲区
    primary_command_buffers.clear();
    secondary_command_buffers.clear();

    // Destroy command pool
    if (handle) {
        device.handle().destroyCommandPool(handle);
    }
}

vk_device& vk_command_pool::get_device()
{
    return device;
}

vk::CommandPool vk_command_pool::get_handle() const
{
    return handle;
}

uint32_t vk_command_pool::get_queue_family_index() const
{
    return queue_family_index;
}

vk_render_frame* vk_command_pool::get_render_frame()
{
    return render_frame;
}

size_t vk_command_pool::get_thread_index() const
{
    return thread_index;
}

void vk_command_pool::reset_pool()
{
    switch (reset_mode) {
        case vk_command_buffer::reset_mode::ResetIndividually:
            reset_command_buffers();
            break;

        case vk_command_buffer::reset_mode::ResetPool:
            device.handle().resetCommandPool(handle);
            reset_command_buffers();
            break;

        case vk_command_buffer::reset_mode::AlwaysAllocate:
            primary_command_buffers.clear();
            active_primary_command_buffer_count = 0;
            secondary_command_buffers.clear();
            active_secondary_command_buffer_count = 0;
            break;

        default:
            throw std::runtime_error("未知的 reset 模式");
    }
}

vk_command_buffer& vk_command_pool::request_command_buffer(vk::CommandBufferLevel level)
{
    if (level == vk::CommandBufferLevel::ePrimary) {
        if (active_primary_command_buffer_count < primary_command_buffers.size()) {
            return *primary_command_buffers[active_primary_command_buffer_count++];
        }

        primary_command_buffers.emplace_back(std::make_unique<vk_command_buffer>(*this, level));

        active_primary_command_buffer_count++;

        return *primary_command_buffers.back();
    } else {
        if (active_secondary_command_buffer_count < secondary_command_buffers.size()) {
            return *secondary_command_buffers[active_secondary_command_buffer_count++];
        }

        secondary_command_buffers.emplace_back(std::make_unique<vk_command_buffer>(*this, level));

        active_secondary_command_buffer_count++;

        return *secondary_command_buffers.back();
    }
}

vk_command_buffer::reset_mode vk_command_pool::get_reset_mode() const
{
    return reset_mode;
}

void vk_command_pool::reset_command_buffers()
{
    for (auto& cmd_buf: primary_command_buffers) {
        cmd_buf->reset(reset_mode);
    }
    active_primary_command_buffer_count = 0;

    for (auto& cmd_buf: secondary_command_buffers) {
        cmd_buf->reset(reset_mode);
    }
    active_secondary_command_buffer_count = 0;
}