/**
 * @File VkCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "volk.h"

#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <vk_mem_alloc.h>

#include <cstdio>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#define VKBP_DISABLE_WARNINGS() __pragma(warning(push, 0))
#define VKBP_ENABLE_WARNINGS() __pragma(warning(pop))

#define DEFAULT_FENCE_TIMEOUT 100000000000        // Default fence timeout in nanoseconds

template<class T>
using BindingMap = std::map<uint32_t, std::map<uint32_t, T>>;

class VulkanException : public std::runtime_error
{
public:
    explicit VulkanException(vk::Result result, const std::string& msg = "Vulkan error") :
        result{result},
        std::runtime_error{msg}
    {
        error_message = std::string(std::runtime_error::what())
                        + std::string{" : "} + vk::to_string(result);
    }

    [[nodiscard]] const char* what() const noexcept override
    {
        return error_message.c_str();
    }

    vk::Result result;

private:
    std::string error_message;
};

#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#define LOGGER_FORMAT "[%^%l%$] %v"
#define PROJECT_NAME "VulkanSamples"

// Mainly for IDEs
#ifndef ROOT_PATH_SIZE
#	define ROOT_PATH_SIZE 0
#endif

#define __FILENAME__ (static_cast<const char *>(__FILE__) + ROOT_PATH_SIZE)

#define LOGI(...) spdlog::info(__VA_ARGS__);
#define LOGW(...) spdlog::warn(__VA_ARGS__);
#define LOGE(...) spdlog::error("[{}:{}] {}", __FILENAME__, __LINE__, fmt::format(__VA_ARGS__));
#define LOGD(...) spdlog::debug(__VA_ARGS__);

#define VK_CHECK(x)                                                 \
    do                                                              \
    {                                                               \
        vk::Result err = static_cast<vk::Result>(x);                \
        if (err != vk::Result::eSuccess)                            \
        {                                                           \
            LOGE("Detected Vulkan error: {}", vk::to_string(err));  \
            abort();                                                \
        }                                                           \
    } while (0)


inline bool is_depth_only_format(vk::Format format)
{
    return format == vk::Format::eD16Unorm ||
           format == vk::Format::eD32Sfloat;
}

inline bool is_depth_stencil_format(vk::Format format)
{
    return format == vk::Format::eD16UnormS8Uint ||
           format == vk::Format::eD24UnormS8Uint ||
           format == vk::Format::eD32SfloatS8Uint;
}

inline bool is_depth_format(vk::Format format)
{
    return is_depth_only_format(format) || is_depth_stencil_format(format);
}

inline vk::Format get_suitable_depth_format(vk::PhysicalDevice physical_device,
                                            bool depth_only = false,
                                            const std::vector<vk::Format>&
                                            depth_format_priority_list = {vk::Format::eD32Sfloat,
                                                                          vk::Format::eD24UnormS8Uint,
                                                                          vk::Format::eD16Unorm})
{
    vk::Format depth_format{vk::Format::eUndefined};

    for (auto& format: depth_format_priority_list) {
        if (depth_only && !is_depth_only_format(format)) {
            continue;
        }

        vk::FormatProperties properties;
        physical_device.getFormatProperties(format, &properties);

        // optimalTiling 必须要支持深度/模板附件
        if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            depth_format = format;
            break;
        }
    }

    if (depth_format != vk::Format::eUndefined) {
        LOGI("深度格式为: {}", to_string(depth_format));
        return depth_format;
    }

    throw std::runtime_error("没有适合的深度格式");
}

struct LoadStoreInfo
{
    vk::AttachmentLoadOp  load_op  = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp store_op = vk::AttachmentStoreOp::eStore;
};

struct BufferMemoryBarrier
{
    vk::PipelineStageFlags src_stage_mask  = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::PipelineStageFlags dst_stage_mask  = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags        src_access_mask = {};
    vk::AccessFlags        dst_access_mask = {};
};

struct ImageMemoryBarrier
{
    vk::PipelineStageFlags src_stage_mask   = vk::PipelineStageFlagBits::eBottomOfPipe;
    vk::PipelineStageFlags dst_stage_mask   = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::AccessFlags        src_access_mask;
    vk::AccessFlags        dst_access_mask;
    vk::ImageLayout        old_layout       = vk::ImageLayout::eUndefined;
    vk::ImageLayout        new_layout       = vk::ImageLayout::eUndefined;
    uint32_t               old_queue_family = VK_QUEUE_FAMILY_IGNORED;
    uint32_t               new_queue_family = VK_QUEUE_FAMILY_IGNORED;
};

inline vk::Sampler create_sampler(vk::Device device, vk::Filter filter,
                                  vk::SamplerAddressMode sampler_address_mode,
                                  float max_anisotropy, float max_LOD)
{
    vk::SamplerCreateInfo sampler_create_info;

    sampler_create_info.magFilter               = filter;
    sampler_create_info.minFilter               = filter;
    sampler_create_info.mipmapMode              = vk::SamplerMipmapMode::eLinear;
    sampler_create_info.addressModeU            = sampler_address_mode;
    sampler_create_info.addressModeV            = sampler_address_mode;
    sampler_create_info.addressModeW            = sampler_address_mode;
    sampler_create_info.mipLodBias              = 0.0f;
    sampler_create_info.anisotropyEnable        = (1.0f < max_anisotropy);
    sampler_create_info.maxAnisotropy           = max_anisotropy;
    sampler_create_info.compareEnable           = false;
    sampler_create_info.compareOp               = vk::CompareOp::eNever;
    sampler_create_info.minLod                  = 0.0f;
    sampler_create_info.maxLod                  = max_LOD;
    sampler_create_info.borderColor             = vk::BorderColor::eFloatOpaqueWhite;
    sampler_create_info.unnormalizedCoordinates = false;

    return device.createSampler(sampler_create_info);
}

inline vk::ImageAspectFlags get_image_aspect_flags(vk::ImageUsageFlagBits usage, vk::Format format)
{
    vk::ImageAspectFlags image_aspect_flags;
    switch (usage) {
        case vk::ImageUsageFlagBits::eColorAttachment:
            assert(!is_depth_format(format));
            image_aspect_flags = vk::ImageAspectFlagBits::eColor;
            break;
        case vk::ImageUsageFlagBits::eDepthStencilAttachment:
            assert(is_depth_format(format));
            image_aspect_flags = vk::ImageAspectFlagBits::eDepth;
            // Stencil aspect should only be set on depth + stencil formats
            if (is_depth_stencil_format(format)) {
                image_aspect_flags |= vk::ImageAspectFlagBits::eStencil;
            }
            break;
        default:
            assert(false);
    }

    return image_aspect_flags;
}