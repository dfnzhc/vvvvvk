/**
 * @File CommandBuffer.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"

class vk_command_pool;

class vk_command_buffer : public vk_unit<vk::CommandBuffer>
{
public:
    enum class reset_mode
    {
        ResetPool,
        ResetIndividually,
        AlwaysAllocate,
    };

    vk_command_buffer(vk_command_pool& command_pool, vk::CommandBufferLevel level);
    vk_command_buffer(vk_command_buffer&& other) noexcept;
    ~vk_command_buffer() override;

    vk_command_buffer(const vk_command_buffer&) = delete;
    vk_command_buffer& operator=(const vk_command_buffer&) = delete;
    vk_command_buffer& operator=(vk_command_buffer&&) = delete;

    /**
	 * @brief 将命令缓冲区重置到可记录的状态
	 * @param reset_mode 如何重置缓冲区，应与池分配缓冲区时使用的缓冲区相匹配
	 */
    vk::Result reset(reset_mode reset_mode);

private:
    const vk::CommandBufferLevel level = {};
    vk_command_pool              & command_pool;
};