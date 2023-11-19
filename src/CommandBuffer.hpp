﻿/**
 * @File CommandBuffer.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "ImageView.hpp"
#include "Renderpass.hpp"
#include "Framebuffer.hpp"

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

    vk::Result begin(vk::CommandBufferUsageFlags flags, vk_command_buffer* primary_cmd_buf = nullptr);
    vk::Result begin(vk::CommandBufferUsageFlags flags, const vk_renderpass* render_pass,
                     const vk_framebuffer* framebuffer, uint32_t subpass_index);

    vk::Result end();
    
    // @formatter:off
    void copy_buffer(const vk_buffer& src_buffer, const vk_buffer& dst_buffer, vk::DeviceSize size);
    void copy_image(const vk_image& src_img, const vk_image& dst_img, const std::vector<vk::ImageCopy>& regions);
    
    void copy_buffer_to_image(const vk_buffer& buffer, const vk_image& image, const std::vector<vk::BufferImageCopy>& regions);
    void copy_image_to_buffer(const vk_image& image, vk::ImageLayout image_layout, const vk_buffer& buffer, const std::vector<vk::BufferImageCopy>& regions);
    // @formatter:on
    void image_memory_barrier(const vk_image_view& image_view, const ImageMemoryBarrier& memory_barrier) const;

private:
    const vk::CommandBufferLevel level = {};
    vk_command_pool& command_pool;
};