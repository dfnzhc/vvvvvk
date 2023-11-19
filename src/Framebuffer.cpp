/**
 * @File Framebuffer.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "Framebuffer.hpp"
#include "Device.hpp"
#include "ImageView.hpp"
#include "RenderTarget.hpp"
#include "Helpers.hpp"

vk_framebuffer::vk_framebuffer(vk_device& device, const vk_render_target& render_target, vk::RenderPass render_pass) :
    device{device},
    extent{render_target.get_extent()}
{
    std::vector<vk::ImageView> attachments;

    for (auto& view: render_target.get_views()) {
        attachments.emplace_back(view.handle());
    }

    vk::FramebufferCreateInfo create_info;

    create_info.renderPass      = render_pass;
    create_info.attachmentCount = to_u32(attachments.size());
    create_info.pAttachments    = attachments.data();
    create_info.width           = extent.width;
    create_info.height          = extent.height;
    create_info.layers          = 1;

    VK_CHECK(device.handle().createFramebuffer(&create_info, nullptr, &handle));
}

vk_framebuffer::vk_framebuffer(vk_framebuffer&& other) :
    device{other.device},
    handle{other.handle},
    extent{other.extent}
{
    other.handle = VK_NULL_HANDLE;
}

vk_framebuffer::~vk_framebuffer()
{
    if (handle != VK_NULL_HANDLE) {
        device.handle().destroyFramebuffer(handle, nullptr);
    }
}

vk::Framebuffer vk_framebuffer::get_handle() const
{
    return handle;
}

const vk::Extent2D& vk_framebuffer::get_extent() const
{
    return extent;
}