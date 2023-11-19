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
