/**
 * @File CommandBufferPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "CommandBuffer.hpp"

class vk_render_frame;

class vk_device;

class vk_command_pool
{
public:
    vk_command_pool(vk_device& device,
                    uint32_t queue_family_index,
                    vk_render_frame* render_frame = nullptr,
                    size_t thread_index = 0,
                    vk_command_buffer::reset_mode reset_mode = vk_command_buffer::reset_mode::ResetPool);
    
    vk_command_pool(vk_command_pool&& other) noexcept ;
    ~vk_command_pool();

    vk_command_pool(const vk_command_pool&) = delete;
    vk_command_pool& operator=(const vk_command_pool&) = delete;
    vk_command_pool& operator=(vk_command_pool&&) = delete;

    vk_device& get_device();
    vk::CommandPool get_handle() const;
    uint32_t get_queue_family_index() const;
    vk_render_frame* get_render_frame();
    vk_command_buffer::reset_mode get_reset_mode() const;
    size_t get_thread_index() const;
    vk_command_buffer& request_command_buffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary);
    void reset_pool();

private:
    void reset_command_buffers();

private:
    vk_device& device;
    vk::CommandPool handle = nullptr;
    vk_render_frame* render_frame = nullptr;

    size_t                                          thread_index                          = 0;
    uint32_t                                        queue_family_index                    = 0;
    std::vector<std::unique_ptr<vk_command_buffer>> primary_command_buffers;
    uint32_t                                        active_primary_command_buffer_count   = 0;
    std::vector<std::unique_ptr<vk_command_buffer>> secondary_command_buffers;
    uint32_t                                        active_secondary_command_buffer_count = 0;
    vk_command_buffer::reset_mode                   reset_mode                            = vk_command_buffer::reset_mode::ResetPool;
};