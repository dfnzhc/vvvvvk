/**
 * @File SwapChain.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include <set>
#include "VkCommon.hpp"
#include "Deivce.hpp"

struct swapchain_properties
{
    vk::SwapchainKHR                old_swapchain;
    uint32_t                        image_count{3};
    vk::Extent2D                    extent;
    vk::SurfaceFormatKHR            surface_format;
    uint32_t                        array_layers;
    vk::ImageUsageFlags             image_usage;
    vk::SurfaceTransformFlagBitsKHR pre_transform;
    vk::CompositeAlphaFlagBitsKHR   composite_alpha;
    vk::PresentModeKHR              present_mode;
};

class vk_swapchain
{
public:
    /**
     * @brief 创建新大小的交换链，保留旧的配置
     */
    vk_swapchain(vk_swapchain& old_swapchain, const vk::Extent2D& extent);

    /**
     * @brief 创建图像数量的交换链，保留旧的配置
     */
    vk_swapchain(vk_swapchain& old_swapchain, uint32_t image_count);

    /**
     * @brief 创建新的图像用途的交换链，保留旧的配置
     */
    vk_swapchain(vk_swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags);

    /**
     * @brief 创建新的图像大小及变换的交换链，保留旧的配置
     */
    vk_swapchain(vk_swapchain& swapchain, const vk::Extent2D& extent, vk::SurfaceTransformFlagBitsKHR transform);

    /**
     * @brief 创建交换链
     */
    // @formatter:off
    vk_swapchain(vk_device                               &device,
                 vk::SurfaceKHR                           surface,
                 vk::PresentModeKHR                       present_mode,
                 const std::vector<vk::PresentModeKHR>   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
                 const std::vector<vk::SurfaceFormatKHR> &surface_format_priority_list = {{vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear},
                                                                                          {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}},
                 const vk::Extent2D                      &extent                       = {},
                 uint32_t                                 image_count                  = 3,
                 vk::SurfaceTransformFlagBitsKHR          transform                    = vk::SurfaceTransformFlagBitsKHR::eIdentity,
                 const std::set<vk::ImageUsageFlagBits>  &image_usage_flags            = {vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eTransferSrc},
                 vk::SwapchainKHR                         old_swapchain                = nullptr);
     // @formatter:on

    vk_swapchain(vk_swapchain&& other) noexcept;

    ~vk_swapchain();

    vk_swapchain(const vk_swapchain&) = delete;
    vk_swapchain& operator=(const vk_swapchain&) = delete;
    vk_swapchain& operator=(vk_swapchain&&) = delete;

    // @formatter:off
    std::pair<vk::Result, uint32_t> acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence = nullptr) const;
    // @formatter:on

    bool is_valid() const;

    const vk_device& device() const;

    vk::SwapchainKHR handle() const;

    vk::SurfaceKHR surface() const;

    const vk::Extent2D& extent() const;

    vk::Format format() const;

    const std::vector<vk::Image>& images() const;

    vk::ImageUsageFlags usage() const;

    vk::PresentModeKHR present_mode() const;

    vk::SurfaceTransformFlagBitsKHR transform() const;

private:
    vk_device& device_;

    vk::SurfaceKHR surface_;

    vk::SwapchainKHR handle_;

    std::vector<vk::Image> images_;

    std::vector<vk::SurfaceFormatKHR> surface_formats_;

    std::vector<vk::PresentModeKHR> present_modes_;

    swapchain_properties properties_;

    // 按优先度排序的一组显示模式
    std::vector<vk::PresentModeKHR> present_mode_priority_list_;

    // 按优先度排序的一组表面格式
    std::vector<vk::SurfaceFormatKHR> surface_format_priority_list_;

    std::set<vk::ImageUsageFlagBits> image_usage_flags_;
};