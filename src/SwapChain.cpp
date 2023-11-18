/**
 * @File SwapChain.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "SwapChain.hpp"
#include "PhysicalDevice.hpp"

namespace {

template<class T>
constexpr const T& clamp(const T& v, const T& lo, const T& hi)
{
    return (v < lo) ? lo : ((hi < v) ? hi : v);
}

vk::Extent2D choose_extent(vk::Extent2D request_extent,
                           const vk::Extent2D& min_image_extent,
                           const vk::Extent2D& max_image_extent,
                           const vk::Extent2D& current_extent)
{
    if (current_extent.width == 0xFFFFFFFF) {
        return request_extent;
    }

    if (request_extent.width < 1 || request_extent.height < 1) {
        LOGW("不支持的交换链图像大小 ({}, {}), 大小设为 ({}, {}).",
             request_extent.width, request_extent.height,
             current_extent.width, current_extent.height);
        return current_extent;
    }

    request_extent.width  = clamp(request_extent.width, min_image_extent.width, max_image_extent.width);
    request_extent.height = clamp(request_extent.height, min_image_extent.height, max_image_extent.height);

    return request_extent;
}

vk::PresentModeKHR choose_present_mode(vk::PresentModeKHR request_present_mode,
                                       const std::vector<vk::PresentModeKHR>& available_present_modes,
                                       const std::vector<vk::PresentModeKHR>& present_mode_priority_list)
{
    // 从可用显示模式中找到所请求的
    auto const present_mode_it = std::find(available_present_modes.begin(), available_present_modes.end(),
                                           request_present_mode);
    if (present_mode_it == available_present_modes.end()) {
        auto const chosen_present_mode_it =
                       std::find_if(present_mode_priority_list.begin(), present_mode_priority_list.end(),
                                    [&available_present_modes](vk::PresentModeKHR present_mode) {
                                        return
                                            std::find(available_present_modes.begin(), available_present_modes.end(),
                                                      present_mode) != available_present_modes.end();
                                    });

        vk::PresentModeKHR const chosen_present_mode = (chosen_present_mode_it != present_mode_priority_list.end())
                                                       ? *chosen_present_mode_it : vk::PresentModeKHR::eFifo;

        LOGW("不支持的显示模式 '{}'. 选择的显示模式为 '{}'.", vk::to_string(request_present_mode),
             vk::to_string(chosen_present_mode));
        return chosen_present_mode;
    } else {
        LOGI("选择的显示模式为: '{}'", to_string(request_present_mode));
        return request_present_mode;
    }
}

vk::SurfaceFormatKHR choose_surface_format(const vk::SurfaceFormatKHR requested_surface_format,
                                           const std::vector<vk::SurfaceFormatKHR>& available_surface_formats,
                                           const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list)
{
    // 从可用表面格式中找到所请求的
    auto const surface_format_it = std::find(available_surface_formats.begin(), available_surface_formats.end(),
                                             requested_surface_format);

    // 如果没找到那么从优先列表中查找
    if (surface_format_it != available_surface_formats.end()) {
        LOGI("交换链格式为: {}",
             vk::to_string(requested_surface_format.format) + ", " +
             vk::to_string(requested_surface_format.colorSpace));
        return requested_surface_format;
    }
    auto const chosen_surface_format_it =
                   std::find_if(surface_format_priority_list.begin(), surface_format_priority_list.end(),
                                [&available_surface_formats](vk::SurfaceFormatKHR surface_format) {
                                    return std::find(available_surface_formats.begin(),
                                                     available_surface_formats.end(), surface_format) !=
                                           available_surface_formats.end();
                                });

    // 如果还没找到，使用第一个可用格式
    const auto& chosen_surface_format = (chosen_surface_format_it != surface_format_priority_list.end())
                                        ? *chosen_surface_format_it : available_surface_formats[0];

    LOGW("不支持的交换链格式 ({}). 选择的格式为 ({}).",
         vk::to_string(requested_surface_format.format) + ", " + vk::to_string(requested_surface_format.colorSpace),
         vk::to_string(chosen_surface_format.format) + ", " + vk::to_string(chosen_surface_format.colorSpace));
    return chosen_surface_format;
}

vk::SurfaceTransformFlagBitsKHR choose_transform(vk::SurfaceTransformFlagBitsKHR request_transform,
                                                 vk::SurfaceTransformFlagsKHR supported_transform,
                                                 vk::SurfaceTransformFlagBitsKHR current_transform)
{
    if (request_transform & supported_transform) {
        return request_transform;
    }

    LOGW("不支持的变换形式 ({}). 选择的变换为 ({}).", vk::to_string(request_transform),
         vk::to_string(current_transform));
    return current_transform;
}

vk::CompositeAlphaFlagBitsKHR choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR request_composite_alpha,
                                                     vk::CompositeAlphaFlagsKHR supported_composite_alpha)
{
    if (request_composite_alpha & supported_composite_alpha) {
        return request_composite_alpha;
    }

    static const std::vector<vk::CompositeAlphaFlagBitsKHR> composite_alpha_priority_list = {
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
        vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
        vk::CompositeAlphaFlagBitsKHR::eInherit};

    auto const chosen_composite_alpha_it =
                   std::find_if(composite_alpha_priority_list.begin(),
                                composite_alpha_priority_list.end(),
                                [&supported_composite_alpha](vk::CompositeAlphaFlagBitsKHR composite_alpha) {
                                    return composite_alpha & supported_composite_alpha;
                                });
    if (chosen_composite_alpha_it == composite_alpha_priority_list.end()) {
        throw std::runtime_error("没有合适的透明度组合方法.");
    } else {
        LOGW("不支持交换链透明度组合方式 '{}'. 选择为 '{}'.",
             vk::to_string(request_composite_alpha), vk::to_string(*chosen_composite_alpha_it));
        return *chosen_composite_alpha_it;
    }
}

bool validate_format_feature(vk::ImageUsageFlagBits image_usage, vk::FormatFeatureFlags supported_features)
{
    return (image_usage != vk::ImageUsageFlagBits::eStorage) ||
           (supported_features & vk::FormatFeatureFlagBits::eStorageImage);
}

std::set<vk::ImageUsageFlagBits> choose_image_usage(const std::set<vk::ImageUsageFlagBits>& requested_image_usage_flags,
                                                    vk::ImageUsageFlags supported_image_usage,
                                                    vk::FormatFeatureFlags supported_features)
{
    std::set<vk::ImageUsageFlagBits> validated_image_usage_flags;
    for (auto                        flag: requested_image_usage_flags) {
        if ((flag & supported_image_usage) && validate_format_feature(flag, supported_features)) {
            validated_image_usage_flags.insert(flag);
        } else {
            LOGW("请求了不支持的交换链图像用途 ({}).", vk::to_string(flag));
        }
    }

    if (validated_image_usage_flags.empty()) {
        // Pick the first format from list of defaults, if supported
        static const std::vector<vk::ImageUsageFlagBits> image_usage_priority_list = {
            vk::ImageUsageFlagBits::eColorAttachment, vk::ImageUsageFlagBits::eStorage,
            vk::ImageUsageFlagBits::eSampled, vk::ImageUsageFlagBits::eTransferDst};

        auto const priority_list_it =
                       std::find_if(image_usage_priority_list.begin(),
                                    image_usage_priority_list.end(),
                                    [&supported_image_usage, &supported_features](auto const image_usage) {
                                        return ((image_usage & supported_image_usage) &&
                                                validate_format_feature(image_usage, supported_features));
                                    });
        if (priority_list_it != image_usage_priority_list.end()) {
            validated_image_usage_flags.insert(*priority_list_it);
        }
    }

    if (validated_image_usage_flags.empty()) {
        throw std::runtime_error("没有发现适合的图像用途.");
    } else {
        std::string usage_list;

        for (vk::ImageUsageFlagBits image_usage: validated_image_usage_flags) {
            usage_list += to_string(image_usage) + " ";
        }
        LOGI("设置的交换链图像用途为: {}", usage_list);
    }

    return validated_image_usage_flags;
}

vk::ImageUsageFlags composite_image_flags(std::set<vk::ImageUsageFlagBits>& image_usage_flags)
{
    vk::ImageUsageFlags image_usage;

    for (auto flag: image_usage_flags) {
        image_usage |= flag;
    }
    return image_usage;
}
}        // namespace


vk_swapchain::vk_swapchain(vk_swapchain& old_swapchain, const vk::Extent2D& extent) :
    vk_swapchain{old_swapchain.device_,
                 old_swapchain.surface_,
                 old_swapchain.properties_.present_mode,
                 old_swapchain.present_mode_priority_list_,
                 old_swapchain.surface_format_priority_list_,
                 extent,
                 old_swapchain.properties_.image_count,
                 old_swapchain.properties_.pre_transform,
                 old_swapchain.image_usage_flags_,
                 old_swapchain.handle()} {}

vk_swapchain::vk_swapchain(vk_swapchain& old_swapchain, const uint32_t image_count) :
    vk_swapchain{old_swapchain.device_,
                 old_swapchain.surface_,
                 old_swapchain.properties_.present_mode,
                 old_swapchain.present_mode_priority_list_,
                 old_swapchain.surface_format_priority_list_,
                 old_swapchain.properties_.extent,
                 image_count,
                 old_swapchain.properties_.pre_transform,
                 old_swapchain.image_usage_flags_,
                 old_swapchain.handle()} {}

vk_swapchain::vk_swapchain(vk_swapchain& old_swapchain, const std::set<vk::ImageUsageFlagBits>& image_usage_flags) :
    vk_swapchain{old_swapchain.device_,
                 old_swapchain.surface_,
                 old_swapchain.properties_.present_mode,
                 old_swapchain.present_mode_priority_list_,
                 old_swapchain.surface_format_priority_list_,
                 old_swapchain.properties_.extent,
                 old_swapchain.properties_.image_count,
                 old_swapchain.properties_.pre_transform,
                 image_usage_flags,
                 old_swapchain.handle()} {}

vk_swapchain::vk_swapchain(vk_swapchain& old_swapchain, const vk::Extent2D& extent,
                           const vk::SurfaceTransformFlagBitsKHR transform) :
    vk_swapchain{old_swapchain.device_,
                 old_swapchain.surface_,
                 old_swapchain.properties_.present_mode,
                 old_swapchain.present_mode_priority_list_,
                 old_swapchain.surface_format_priority_list_,
                 extent,
                 old_swapchain.properties_.image_count,
                 transform,
                 old_swapchain.image_usage_flags_,
                 old_swapchain.handle()} {}

vk_swapchain::vk_swapchain(vk_device& device,
                           vk::SurfaceKHR surface,
                           const vk::PresentModeKHR present_mode,
                           const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
                           const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list,
                           const vk::Extent2D& extent,
                           const uint32_t image_count,
                           const vk::SurfaceTransformFlagBitsKHR transform,
                           const std::set<vk::ImageUsageFlagBits>& image_usage_flags,
                           vk::SwapchainKHR old_swapchain) :
    device_{device},
    surface_{surface}
{
    this->present_mode_priority_list_   = present_mode_priority_list;
    this->surface_format_priority_list_ = surface_format_priority_list;

    surface_formats_ = device.get_gpu().handle().getSurfaceFormatsKHR(surface);

    LOGI("Surface 支持以下的表面格式:");
    for (auto& surface_format: surface_formats_) {
        LOGI("  \t{}", vk::to_string(surface_format.format) + ", " + vk::to_string(surface_format.colorSpace));
    }

    present_modes_ = device.get_gpu().handle().getSurfacePresentModesKHR(surface);
    LOGI("Surface 支持以下的显示模式:");
    for (auto& pm: present_modes_) {
        LOGI("  \t{}", to_string(pm));
    }

    // 以 Surface 的能力，选择最佳的特性
    const auto& surface_capabilities = device.get_gpu().handle().getSurfaceCapabilitiesKHR(surface);
    const auto& format_properties    = device.get_gpu().handle().getFormatProperties(properties_.surface_format.format);

    // @formatter:off
    this->image_usage_flags_ = choose_image_usage(image_usage_flags, surface_capabilities.supportedUsageFlags, format_properties.optimalTilingFeatures);

    properties_.image_count     = clamp(image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount ? surface_capabilities.maxImageCount : std::numeric_limits<uint32_t>::max());
    properties_.extent          = choose_extent(extent, surface_capabilities.minImageExtent, surface_capabilities.maxImageExtent, surface_capabilities.currentExtent);
    properties_.array_layers    = 1;
    properties_.surface_format  = choose_surface_format(properties_.surface_format, surface_formats_, surface_format_priority_list);
    properties_.image_usage     = composite_image_flags(this->image_usage_flags_);
    properties_.pre_transform   = choose_transform(transform, surface_capabilities.supportedTransforms, surface_capabilities.currentTransform);
    properties_.composite_alpha = choose_composite_alpha(vk::CompositeAlphaFlagBitsKHR::eInherit, surface_capabilities.supportedCompositeAlpha);

    properties_.old_swapchain = old_swapchain;
    properties_.present_mode  = present_mode;
    // @formatter:on

    // Revalidate the present mode and surface format
    // @formatter:off
    properties_.present_mode   = choose_present_mode(properties_.present_mode, present_modes_, present_mode_priority_list);
    properties_.surface_format = choose_surface_format(properties_.surface_format, surface_formats_, surface_format_priority_list);
    // @formatter:on

    const vk::SwapchainCreateInfoKHR create_info({},
                                                 surface,
                                                 properties_.image_count,
                                                 properties_.surface_format.format,
                                                 properties_.surface_format.colorSpace,
                                                 properties_.extent,
                                                 properties_.array_layers,
                                                 properties_.image_usage,
                                                 {},
                                                 {},
                                                 properties_.pre_transform,
                                                 properties_.composite_alpha,
                                                 properties_.present_mode,
                                                 {},
                                                 properties_.old_swapchain);

    handle_ = device.handle().createSwapchainKHR(create_info);
    images_ = device.handle().getSwapchainImagesKHR(handle_);
}

vk_swapchain::~vk_swapchain()
{
    if (handle_) {
        LOGI("交换链 '{}' 已经清除", reinterpret_cast<std::size_t>(&handle_));
        device_.handle().destroySwapchainKHR(handle_);
    }
}

vk_swapchain::vk_swapchain(vk_swapchain&& other) noexcept :
    device_{other.device_},
    surface_{std::exchange(other.surface_, nullptr)},
    handle_{std::exchange(other.handle_, nullptr)},
    images_{std::exchange(other.images_, {})},
    surface_formats_{std::exchange(other.surface_formats_, {})},
    present_modes_{std::exchange(other.present_modes_, {})},
    properties_{std::exchange(other.properties_, {})},
    present_mode_priority_list_{std::exchange(other.present_mode_priority_list_, {})},
    surface_format_priority_list_{std::exchange(other.surface_format_priority_list_, {})},
    image_usage_flags_{std::move(other.image_usage_flags_)} {}

bool vk_swapchain::is_valid() const
{
    return !!handle_;
}

const vk_device& vk_swapchain::device() const
{
    return device_;
}

vk::SwapchainKHR vk_swapchain::handle() const
{
    return handle_;
}

std::pair<vk::Result, uint32_t>
vk_swapchain::acquire_next_image(vk::Semaphore image_acquired_semaphore, vk::Fence fence) const
{
    vk::ResultValue<uint32_t> rv = device_.handle()
        .acquireNextImageKHR(handle_, std::numeric_limits<uint64_t>::max(), image_acquired_semaphore, fence);
    return std::make_pair(rv.result, rv.value);
}

const vk::Extent2D& vk_swapchain::extent() const
{
    return properties_.extent;
}

vk::Format vk_swapchain::format() const
{
    return properties_.surface_format.format;
}

const std::vector<vk::Image>& vk_swapchain::images() const
{
    return images_;
}

vk::SurfaceTransformFlagBitsKHR vk_swapchain::transform() const
{
    return properties_.pre_transform;
}

vk::SurfaceKHR vk_swapchain::surface() const
{
    return surface_;
}

vk::ImageUsageFlags vk_swapchain::usage() const
{
    return properties_.image_usage;
}

vk::PresentModeKHR vk_swapchain::present_mode() const
{
    return properties_.present_mode;
}