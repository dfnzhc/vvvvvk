/**
 * @File VkUtils.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/23
 * @Brief 
 */

#include "VkUtils.hpp"

vk::ImageMemoryBarrier makeImageMemoryBarrier(vk::Image img,
                                              vk::AccessFlags srcAccess,
                                              vk::AccessFlags dstAccess,
                                              vk::ImageLayout oldLayout,
                                              vk::ImageLayout newLayout,
                                              vk::ImageAspectFlags aspectMask)
{
    vk::ImageMemoryBarrier barrier;
    barrier.srcAccessMask               = srcAccess;
    barrier.dstAccessMask               = dstAccess;
    barrier.oldLayout                   = oldLayout;
    barrier.newLayout                   = newLayout;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                       = img;
    barrier.subresourceRange            = {0};
    barrier.subresourceRange.aspectMask = aspectMask;
    barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
    barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;

    return barrier;
}

vk::AccessFlags getAccessFlags(vk::ImageLayout layout)
{
    switch (layout) {
        // @formatter:off
        case vk::ImageLayout::eUndefined:
        case vk::ImageLayout::ePresentSrcKHR:
            return {};
            
        case vk::ImageLayout::ePreinitialized:                  
            return vk::AccessFlagBits::eHostWrite;
            
        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;
            
        case vk::ImageLayout::eDepthAttachmentOptimal:
        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
            
        case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
            return vk::AccessFlagBits::eFragmentShadingRateAttachmentReadKHR;
            
        case vk::ImageLayout::eShaderReadOnlyOptimal:           
            return vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eInputAttachmentRead;
        
        case vk::ImageLayout::eTransferDstOptimal:              
            return vk::AccessFlagBits::eTransferWrite;
            
        case vk::ImageLayout::eTransferSrcOptimal:              
            return vk::AccessFlagBits::eTransferRead;
            
        // @formatter:on
        case vk::ImageLayout::eGeneral:
            assert(false);
            return {};
        default:
            assert(false);
            return {};
    }
}

vk::PipelineStageFlags getPipelineStageFlags(vk::ImageLayout layout)
{
    switch (layout) {
        case vk::ImageLayout::eUndefined:
            return vk::PipelineStageFlagBits::eTopOfPipe;

        case vk::ImageLayout::ePreinitialized:
            return vk::PipelineStageFlagBits::eHost;

        case vk::ImageLayout::eTransferSrcOptimal:
        case vk::ImageLayout::eTransferDstOptimal:
            return vk::PipelineStageFlagBits::eTransfer;

        case vk::ImageLayout::eColorAttachmentOptimal:
            return vk::PipelineStageFlagBits::eColorAttachmentOutput;

        case vk::ImageLayout::eDepthStencilAttachmentOptimal:
            return vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests;

        case vk::ImageLayout::eFragmentShadingRateAttachmentOptimalKHR:
            return vk::PipelineStageFlagBits::eFragmentShadingRateAttachmentKHR;

        case vk::ImageLayout::eShaderReadOnlyOptimal:
            return vk::PipelineStageFlagBits::eVertexShader | vk::PipelineStageFlagBits::eFragmentShader;

        case vk::ImageLayout::ePresentSrcKHR:
            return vk::PipelineStageFlagBits::eBottomOfPipe;

        case vk::ImageLayout::eGeneral:
            assert(false);
            return {};
        default:
            assert(false);
            return {};
    }
}

// @formatter:off
void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::PipelineStageFlags           src_stage_mask,
                             vk::PipelineStageFlags           dst_stage_mask,
                             vk::AccessFlags                  src_access_mask,
                             vk::AccessFlags                  dst_access_mask,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             const vk::ImageSubresourceRange& subresource_range)
{
    vk::ImageMemoryBarrier image_memory_barrier{};
    image_memory_barrier.srcAccessMask       = src_access_mask;
    image_memory_barrier.dstAccessMask       = dst_access_mask;
    image_memory_barrier.oldLayout           = old_layout;
    image_memory_barrier.newLayout           = new_layout;
    image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    image_memory_barrier.image               = image;
    image_memory_barrier.subresourceRange    = subresource_range;

    command_buffer.pipelineBarrier(src_stage_mask, dst_stage_mask, {}, 0, nullptr, 0, nullptr, 1, &image_memory_barrier);
}

void image_layout_transition(vk::CommandBuffer                command_buffer,
                             vk::Image                        image,
                             vk::ImageLayout                  old_layout,
                             vk::ImageLayout                  new_layout,
                             const vk::ImageSubresourceRange& subresource_range)
{
    vk::PipelineStageFlags src_stage_mask  = getPipelineStageFlags(old_layout);
    vk::PipelineStageFlags dst_stage_mask  = getPipelineStageFlags(new_layout);
    vk::AccessFlags        src_access_mask = getAccessFlags(old_layout);
    vk::AccessFlags        dst_access_mask = getAccessFlags(new_layout);

    image_layout_transition(command_buffer, image, src_stage_mask, dst_stage_mask, src_access_mask, dst_access_mask, old_layout, new_layout, subresource_range);
}

// Fixed sub resource on first mip level and layer
void image_layout_transition(vk::CommandBuffer command_buffer,
                             vk::Image         image,
                             vk::ImageLayout   old_layout,
                             vk::ImageLayout   new_layout)
{
    vk::ImageSubresourceRange subresource_range = {};
    subresource_range.aspectMask              = vk::ImageAspectFlagBits::eColor;
    subresource_range.baseMipLevel            = 0;
    subresource_range.levelCount              = 1;
    subresource_range.baseArrayLayer          = 0;
    subresource_range.layerCount              = 1;
    
    image_layout_transition(command_buffer, image, old_layout, new_layout, subresource_range);
}

void image_layout_transition(vk::CommandBuffer                                                      command_buffer,
                             const std::vector<std::pair<vk::Image, vk::ImageSubresourceRange>>&    imagesAndRanges,
                             vk::ImageLayout                                                        old_layout,
                             vk::ImageLayout                                                        new_layout)
{
    vk::PipelineStageFlags src_stage_mask  = getPipelineStageFlags(old_layout);
    vk::PipelineStageFlags dst_stage_mask  = getPipelineStageFlags(new_layout);
    vk::AccessFlags        src_access_mask = getAccessFlags(old_layout);
    vk::AccessFlags        dst_access_mask = getAccessFlags(new_layout);

    std::vector<vk::ImageMemoryBarrier> image_memory_barriers;
    image_memory_barriers.reserve(imagesAndRanges.size());
    for (const auto& [image, range] : imagesAndRanges)
    {
        image_memory_barriers.emplace_back(src_access_mask,
                                           dst_access_mask,
                                           old_layout,
                                           new_layout,
                                           VK_QUEUE_FAMILY_IGNORED,
                                           VK_QUEUE_FAMILY_IGNORED,
                                           image,
                                           range);
    }

    command_buffer.pipelineBarrier(src_stage_mask,
                                   dst_stage_mask,
                                   {},
                                   0, nullptr,
                                   0, nullptr,
                                   static_cast<uint32_t>(image_memory_barriers.size()),
                                   image_memory_barriers.data());
}
// @formatter:on
