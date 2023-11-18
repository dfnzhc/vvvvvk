/**
 * @File VkCommon.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_to_string.hpp>

#include <vk_mem_alloc.h>

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