/**
 * @File Framebuffer.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_render_target;

class vk_framebuffer
{
public:
    vk_framebuffer(vk_device& device, const vk_render_target& render_target, vk::RenderPass render_pass);
    vk_framebuffer(vk_framebuffer&& other);

    ~vk_framebuffer();

    vk_framebuffer(const vk_framebuffer&) = delete;

    vk_framebuffer& operator=(const vk_framebuffer&) = delete;
    vk_framebuffer& operator=(vk_framebuffer&&) = delete;

    vk::Framebuffer get_handle() const;
    const vk::Extent2D& get_extent() const;

private:
    vk_device& device;

    vk::Framebuffer handle{VK_NULL_HANDLE};
    vk::Extent2D    extent{};
};