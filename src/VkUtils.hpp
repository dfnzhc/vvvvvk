/**
 * @File VkUtils.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/23
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

inline uint32_t mipLevels(VkExtent2D extent)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

inline uint32_t mipLevels(VkExtent3D extent)
{
    return static_cast<uint32_t>(std::floor(std::log2(std::max(extent.width, extent.height)))) + 1;
}

// 根据图像的访问信息和布局创建内存屏障
vk::ImageMemoryBarrier makeImageMemoryBarrier(vk::Image image,
                                              vk::AccessFlags srcAccess,
                                              vk::AccessFlags dstAccess,
                                              vk::ImageLayout oldLayout,
                                              vk::ImageLayout newLayout,
                                              vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor);

// @formatter:off
void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::PipelineStageFlags           src_stage_mask,
                             vk::PipelineStageFlags           dst_stage_mask,
                             vk::AccessFlags                  src_access_mask,
                             vk::AccessFlags                  dst_access_mask,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             const vk::ImageSubresourceRange& subresource_range);

void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             const vk::ImageSubresourceRange& subresource_range);

void image_layout_transition(vk::CommandBuffer command_buffer,
                             vk::Image         image,
                             vk::ImageLayout   old_layout,
                             vk::ImageLayout   new_layout);

void image_layout_transition(vk::CommandBuffer                                                      command_buffer,
                             const std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>>&    imagesAndRanges,
                             vk::ImageLayout                                                        old_layout,
                             vk::ImageLayout                                                        new_layout);
// @formatter:on


inline VmaMemoryUsage vkToVmaMemoryUsage(vk::MemoryPropertyFlags flags)
{
    if (flags == vk::MemoryPropertyFlagBits::eDeviceLocal)
        return VMA_MEMORY_USAGE_GPU_ONLY;

    if (flags & vk::MemoryPropertyFlagBits::eHostCoherent)
        return VMA_MEMORY_USAGE_CPU_ONLY;

    if (flags & vk::MemoryPropertyFlagBits::eHostVisible)
        return VMA_MEMORY_USAGE_CPU_TO_GPU;

    return VMA_MEMORY_USAGE_UNKNOWN;
}

