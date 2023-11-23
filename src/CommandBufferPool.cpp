/**
 * @File CommandBufferPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "CommandBufferPool.hpp"
#include "Device.hpp"

vk_command_pool::vk_command_pool(vk_device& d,
                                 uint32_t queue_family_index,
                                 vk_command_buffer::reset_mode reset_mode) :
    device_{d}, reset_mode_{reset_mode}
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
    handle_ = device_.handle().createCommandPool(command_pool_create_info);
}

vk_command_pool::vk_command_pool(vk_command_pool&& other) noexcept :
    device_(other.device_),
    handle_(std::exchange(other.handle_, {})),
    queue_family_index_(std::exchange(other.queue_family_index_, {})),
    primary_command_buffers_{std::move(other.primary_command_buffers_)},
    active_primary_command_buffer_count_(std::exchange(other.active_primary_command_buffer_count_, {})),
    secondary_command_buffers_{std::move(other.secondary_command_buffers_)},
    active_secondary_command_buffer_count_(std::exchange(other.active_secondary_command_buffer_count_, {})),
    reset_mode_(std::exchange(other.reset_mode_, {}))
{
}

vk_command_pool::~vk_command_pool()
{
    // 清楚所有记录的命令缓冲区
    primary_command_buffers_.clear();
    secondary_command_buffers_.clear();

    // Destroy command pool
    if (handle_) {
        device_.handle().destroyCommandPool(handle_);
    }
}

vk_device& vk_command_pool::device()
{
    return device_;
}

vk::CommandPool vk_command_pool::handle() const
{
    return handle_;
}

uint32_t vk_command_pool::queue_family_index() const
{
    return queue_family_index_;
}

void vk_command_pool::reset_pool()
{
    switch (reset_mode_) {
        case vk_command_buffer::reset_mode::ResetIndividually:
            reset_command_buffers();
            break;

        case vk_command_buffer::reset_mode::ResetPool:
            device_.handle().resetCommandPool(handle_);
            reset_command_buffers();
            break;

        case vk_command_buffer::reset_mode::AlwaysAllocate:
            primary_command_buffers_.clear();
            active_primary_command_buffer_count_ = 0;
            secondary_command_buffers_.clear();
            active_secondary_command_buffer_count_ = 0;
            break;

        default:
            throw std::runtime_error("未知的 reset 模式");
    }
}

vk_command_buffer& vk_command_pool::request_command_buffer(vk::CommandBufferLevel level)
{
    if (level == vk::CommandBufferLevel::ePrimary) {
        if (active_primary_command_buffer_count_ < primary_command_buffers_.size()) {
            return *primary_command_buffers_[active_primary_command_buffer_count_++];
        }

        primary_command_buffers_.emplace_back(std::make_unique<vk_command_buffer>(*this, level));

        active_primary_command_buffer_count_++;

        return *primary_command_buffers_.back();
    } else {
        if (active_secondary_command_buffer_count_ < secondary_command_buffers_.size()) {
            return *secondary_command_buffers_[active_secondary_command_buffer_count_++];
        }

        secondary_command_buffers_.emplace_back(std::make_unique<vk_command_buffer>(*this, level));

        active_secondary_command_buffer_count_++;

        return *secondary_command_buffers_.back();
    }
}

vk_command_buffer::reset_mode vk_command_pool::reset_mode() const
{
    return reset_mode_;
}

void vk_command_pool::reset_command_buffers()
{
    for (auto& cmd_buf: primary_command_buffers_) {
        cmd_buf->reset(reset_mode_);
    }
    active_primary_command_buffer_count_ = 0;

    for (auto& cmd_buf: secondary_command_buffers_) {
        cmd_buf->reset(reset_mode_);
    }
    active_secondary_command_buffer_count_ = 0;
}