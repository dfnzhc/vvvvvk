/**
 * @File ImageView.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkUnit.hpp"
#include "VkCommon.hpp"

#include "Image.hpp"

class vk_image_view : public vk_unit<vk::ImageView>
{
public:
    vk_image_view(vk_image& image,
                  vk::ImageViewType view_type,
                  vk::Format format = vk::Format::eUndefined,
                  uint32_t base_mip_level = 0,
                  uint32_t base_array_layer = 0,
                  uint32_t n_mip_levels = 0,
                  uint32_t n_array_layers = 0);

    vk_image_view(vk_image_view&) = delete;
    vk_image_view(vk_image_view&& other);

    ~vk_image_view() override;

    vk_image_view& operator=(const vk_image_view&) = delete;
    vk_image_view& operator=(vk_image_view&&) = delete;

    vk::Format get_format() const;
    const vk_image& get_image() const;
    void set_image(vk_image& image);
    vk::ImageSubresourceLayers get_subresource_layers() const;
    vk::ImageSubresourceRange get_subresource_range() const;

private:
    vk_image* image = nullptr;
    vk::Format                format;
    vk::ImageSubresourceRange subresource_range;
};