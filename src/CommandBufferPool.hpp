/**
 * @File CommandBufferPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "CommandBuffer.hpp"

class vk_device;

class vk_command_pool
{
public:
    vk_command_pool(vk_device& device,
                    uint32_t queue_family_index,
                    vk_command_buffer::reset_mode reset_mode = vk_command_buffer::reset_mode::ResetPool);

    vk_command_pool(vk_command_pool&& other) noexcept;
    ~vk_command_pool();

    vk_command_pool(const vk_command_pool&) = delete;
    vk_command_pool& operator=(const vk_command_pool&) = delete;
    vk_command_pool& operator=(vk_command_pool&&) = delete;

    vk_device& device();
    vk::CommandPool handle() const;
    uint32_t queue_family_index() const;
    vk_command_buffer::reset_mode reset_mode() const;
    vk_command_buffer& request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
    void reset_pool();

private:
    void reset_command_buffers();

private:
    vk_device& device_;
    vk::CommandPool handle_             = nullptr;
    uint32_t        queue_family_index_ = 0;

    std::vector<std::unique_ptr<vk_command_buffer>> primary_command_buffers_;
    uint32_t                                        active_primary_command_buffer_count_   = 0;
    std::vector<std::unique_ptr<vk_command_buffer>> secondary_command_buffers_;
    uint32_t                                        active_secondary_command_buffer_count_ = 0;
    
    vk_command_buffer::reset_mode reset_mode_ = vk_command_buffer::reset_mode::ResetPool;
};