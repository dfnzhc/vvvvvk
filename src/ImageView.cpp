/**
 * @File ImageView.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "ImageView.hpp"
#include "Device.hpp"
#include "Debug.hpp"

#include <vulkan/vulkan_format_traits.hpp>

vk_image_view::vk_image_view(vk_image& img,
                             vk::ImageViewType view_type,
                             vk::Format format,
                             uint32_t mip_level,
                             uint32_t array_layer,
                             uint32_t n_mip_levels,
                             uint32_t n_array_layers) :
    vk_unit{nullptr, &img.device()}, image{&img}, format{format}
{
    if (format == vk::Format::eUndefined) {
        this->format = format = image->get_format();
    }

    subresource_range =
        vk::ImageSubresourceRange((std::string(vk::componentName(format, 0)) == "D") ? vk::ImageAspectFlagBits::eDepth
                                                                                     : vk::ImageAspectFlagBits::eColor,
                                  mip_level,
                                  n_mip_levels == 0 ? image->get_subresource().mipLevel : n_mip_levels,
                                  array_layer,
                                  n_array_layers == 0 ? image->get_subresource().arrayLayer : n_array_layers);

    vk::ImageViewCreateInfo image_view_create_info({}, image->handle(), view_type, format, {}, subresource_range);

    set_handle(device().handle().createImageView(image_view_create_info));

    // Register this image view to its image
    // in order to be notified when it gets moved
    image->get_views().emplace(this);
}

vk_image_view::vk_image_view(vk_image_view&& other) :
    vk_unit{std::move(other)}, image{other.image}, format{other.format}, subresource_range{other.subresource_range}
{
    // Remove old view from image set and add this new one
    auto& views = image->get_views();
    views.erase(&other);
    views.emplace(this);

    other.set_handle(nullptr);
}

vk_image_view::~vk_image_view()
{
    if (handle()) {
        device().handle().destroyImageView(handle());
    }
}

vk::Format vk_image_view::get_format() const
{
    return format;
}

const vk_image& vk_image_view::get_image() const
{
    assert(image && "图像视图引用了无效的图像");
    return *image;
}

void vk_image_view::set_image(vk_image& img)
{
    image = &img;
}

vk::ImageSubresourceLayers vk_image_view::get_subresource_layers() const
{
    return {subresource_range.aspectMask, subresource_range.baseMipLevel,
            subresource_range.baseArrayLayer, subresource_range.layerCount};
}

vk::ImageSubresourceRange vk_image_view::get_subresource_range() const
{
    return subresource_range;
}