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

vk::Result vk_command_buffer::reset(vk_command_buffer::reset_mode reset_mode)
{
    assert(reset_mode == command_pool.get_reset_mode() &&
           "命令缓冲区的重置模式必须与分配它时的池一致");

    if (reset_mode == reset_mode::ResetIndividually) {
        handle().reset(vk::CommandBufferResetFlagBits::eReleaseResources);
    }

    return vk::Result::eSuccess;
}
