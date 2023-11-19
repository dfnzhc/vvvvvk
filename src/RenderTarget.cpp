/**
 * @File RenderTarget.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "RenderTarget.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"

#include "Image.hpp"
#include "ImageView.hpp"

/**
 * @brief 默认的渲染目标：一个颜色图像，一个深度图像
 */
const vk_render_target::CreateFunc vk_render_target::DEFAULT_CREATE_FUNC = [](vk_image&& swapchain_image)
    -> std::unique_ptr<vk_render_target> {
    
    vk::Format depth_format = get_suitable_depth_format(swapchain_image.device().get_gpu().handle());
    vk_image depth_image{swapchain_image.device(), swapchain_image.get_extent(), depth_format,
                         vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eTransientAttachment,
                         VMA_MEMORY_USAGE_GPU_ONLY};

    std::vector<vk_image> images;
    images.push_back(std::move(swapchain_image));
    images.push_back(std::move(depth_image));

    return std::make_unique<vk_render_target>(std::move(images));
};

vk_render_target::vk_render_target(std::vector<vk_image>&& images_) :
    device{images_.back().device()},
    images{std::move(images_)}
{
    assert(!images.empty() && "至少要有一张图像");

    // 所有图像必须是 2D 的
    auto it = std::find_if(images.begin(), images.end(),
                           [](vk_image const& image) { return image.get_type() != vk::ImageType::e2D; });
    if (it != images.end()) {
        throw VulkanException{vk::Result::eErrorInitializationFailed, "图像类型不是 2D 的"};
    }

    extent.width  = images.front().get_extent().width;
    extent.height = images.front().get_extent().height;

    // 所有图像大小需要相同
    it = std::find_if(std::next(images.begin()),
                      images.end(),
                      [this](vk_image const& image) {
                          return (extent.width != image.get_extent().width) ||
                                 (extent.height != image.get_extent().height);
                      });
    if (it != images.end()) {
        throw VulkanException{vk::Result::eErrorInitializationFailed, "图像大小不一致"};
    }

    for (auto& image: images) {
        views.emplace_back(image, vk::ImageViewType::e2D);
        attachments.emplace_back(image.get_format(), image.get_sample_count(), image.get_usage());
    }
}

vk_render_target::vk_render_target(std::vector<vk_image_view>&& image_views) :
    device{image_views.back().get_image().device()},
    views{std::move(image_views)}
{
    assert(!views.empty() && "至少要有一张图像视图");

    const uint32_t mip_level = views.front().get_subresource_range().baseMipLevel;
    extent.width  = views.front().get_image().get_extent().width >> mip_level;
    extent.height = views.front().get_image().get_extent().height >> mip_level;

    // 所有图像视图大小需要相同
    auto it = std::find_if(std::next(views.begin()),
                           views.end(),
                           [this](vk_image_view const& image_view) {
                               const uint32_t mip_level = image_view.get_subresource_range().baseMipLevel;
                               return (extent.width != image_view.get_image().get_extent().width >> mip_level) ||
                                      (extent.height != image_view.get_image().get_extent().height >> mip_level);
                           });
    if (it != views.end()) {
        throw VulkanException{vk::Result::eErrorInitializationFailed, "图像视图大小不一致"};
    }

    for (auto& view: views) {
        const auto& image = view.get_image();
        attachments.emplace_back(image.get_format(), image.get_sample_count(), image.get_usage());
    }
}

const vk::Extent2D& vk_render_target::get_extent() const
{
    return extent;
}

const std::vector<vk_image_view>& vk_render_target::get_views() const
{
    return views;
}

const std::vector<rt_attachment>& vk_render_target::get_attachments() const
{
    return attachments;
}

void vk_render_target::set_input_attachments(std::vector<uint32_t>& input)
{
    input_attachments = input;
}

const std::vector<uint32_t>& vk_render_target::get_input_attachments() const
{
    return input_attachments;
}

void vk_render_target::set_output_attachments(std::vector<uint32_t>& output)
{
    output_attachments = output;
}

const std::vector<uint32_t>& vk_render_target::get_output_attachments() const
{
    return output_attachments;
}

void vk_render_target::set_layout(uint32_t attachment, vk::ImageLayout layout)
{
    attachments[attachment].initial_layout = layout;
}

vk::ImageLayout vk_render_target::get_layout(uint32_t attachment) const
{
    return attachments[attachment].initial_layout;
}