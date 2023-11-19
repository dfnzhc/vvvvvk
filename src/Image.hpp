/**
 * @File Image.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkUnit.hpp"
#include "VkCommon.hpp"
#include <unordered_set>

class vk_image_view;

class vk_image : public vk_unit<vk::Image>
{
public:
    // @formatter:off
    vk_image(vk_device              &device,
             vk::Image               handle,
             const vk::Extent3D     &extent,
             vk::Format              format,
             vk::ImageUsageFlags     image_usage,
             vk::SampleCountFlagBits sample_count = vk::SampleCountFlagBits::e1);

    vk_image(vk_device              &device,
             const vk::Extent3D     &extent,
             vk::Format              format,
             vk::ImageUsageFlags     image_usage,
             VmaMemoryUsage          memory_usage,
             vk::SampleCountFlagBits sample_count       = vk::SampleCountFlagBits::e1,
             uint32_t                mip_levels         = 1,
             uint32_t                array_layers       = 1,
             vk::ImageTiling         tiling             = vk::ImageTiling::eOptimal,
             vk::ImageCreateFlags    flags              = {},
             uint32_t                num_queue_families = 0,
             const uint32_t         *queue_families     = nullptr);
    // @formatter:on

    vk_image(vk_image&& other) noexcept;

    ~vk_image() override;

    vk_image(const vk_image&) = delete;
    
    vk_image& operator=(const vk_image&) = delete;
    vk_image& operator=(vk_image&&) = delete;

    VmaAllocation get_memory() const;

    uint8_t* map();

    void unmap();

    vk::ImageType get_type() const;
    const vk::Extent3D& get_extent() const;
    vk::Format get_format() const;
    vk::SampleCountFlagBits get_sample_count() const;
    vk::ImageUsageFlags get_usage() const;
    vk::ImageTiling get_tiling() const;
    vk::ImageSubresource get_subresource() const;
    uint32_t get_array_layer_count() const;
    std::unordered_set<vk_image_view*>& get_views();

private:
    VmaAllocation                      memory            = VK_NULL_HANDLE;
    vk::ImageType                      type;
    vk::Extent3D                       extent;
    vk::Format                         format;
    vk::ImageUsageFlags                usage;
    vk::SampleCountFlagBits            sample_count;
    vk::ImageTiling                    tiling;
    vk::ImageSubresource               subresource;
    uint32_t                           array_layer_count = 0;
    std::unordered_set<vk_image_view*> views;                            /// HPPImage views referring to this image
    uint8_t* mapped_data = nullptr;
    bool mapped = false;                                                /// Whether it was mapped with vmaMapMemory
};