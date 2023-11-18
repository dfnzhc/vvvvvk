/**
 * @File VkInstance.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "Instance.hpp"
#include "PhysicalDevice.hpp"

namespace {

VkBool32 debugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
                                VkDebugUtilsMessageTypeFlagsEXT message_type,
                                const VkDebugUtilsMessengerCallbackDataEXT* callback_data, void* user_data)
{
    if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        LOGW("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    } else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        LOGE("{} - {}: {}", callback_data->messageIdNumber, callback_data->pMessageIdName, callback_data->pMessage);
    }
    return VK_FALSE;
}


} // namespace 

vk_instance::vk_instance(vk::Instance instance,
                         const std::vector<const char*>& enabled_extensions) :
    handle_{instance}, enabled_extensions_(enabled_extensions)
{
    init_debug_utils();

    if (handle_) {
        query_gpus();
    } else {
        throw std::runtime_error("vk_instance not valid");
    }
}

vk_instance::~vk_instance()
{
    if (destroyDebugUtilsMessengerEXT_) {
        destroyDebugUtilsMessengerEXT_(handle_, debug_messenger_, nullptr);
        debug_messenger_ = VK_NULL_HANDLE;
    }

    if (handle_) {
        handle_.destroy();
    }

    createDebugUtilsMessengerEXT_  = nullptr;
    destroyDebugUtilsMessengerEXT_ = nullptr;
}

const std::vector<const char*>& vk_instance::extensions()
{
    return enabled_extensions_;
}

vk::Instance vk_instance::handle() const
{
    return handle_;
}

bool vk_instance::is_enabled(const char* extension) const
{
    return std::find_if(enabled_extensions_.begin(),
                        enabled_extensions_.end(),
                        [extension](const char* enabled_extension) {
                            return strcmp(extension, enabled_extension) == 0;
                        }) != enabled_extensions_.end();
}

void vk_instance::init_debug_utils()
{
    createDebugUtilsMessengerEXT_  =
        (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(handle_, "vkCreateDebugUtilsMessengerEXT");
    destroyDebugUtilsMessengerEXT_ =
        (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(handle_, "vkDestroyDebugUtilsMessengerEXT");

    if (createDebugUtilsMessengerEXT_ != nullptr) {
        VkDebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        dbg_messenger_create_info.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT       // For debug printf
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT  // GPU info, bug
            | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;   // Invalid usage
        dbg_messenger_create_info.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT            // Other
                                                    |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT       // Violation of spec
                                                    |
                                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;     // Non-optimal use
        dbg_messenger_create_info.pfnUserCallback = debugMessengerCallback;
        dbg_messenger_create_info.pUserData       = this;

        VkDebugUtilsMessengerEXT db;
        createDebugUtilsMessengerEXT_(handle_, &dbg_messenger_create_info, nullptr, &db);
        debug_messenger_ = db;
    }
}

vk_physical_device& vk_instance::get_first_gpu()
{
    assert(!gpus.empty() && "No physical devices were found on the system.");

    // Find a discrete GPU
    for (auto& gpu: gpus) {
        if (gpu->properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            return *gpu;
        }
    }

    // Otherwise just pick the first one
    LOGW("Couldn't find a discrete physical device, picking default GPU");
    return *gpus[0];
}


vk_physical_device& vk_instance::get_suitable_gpu(vk::SurfaceKHR surface)
{
    assert(!gpus.empty() && "No physical devices were found on the system.");

    for (auto& gpu: gpus) {
        if (gpu->properties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
            size_t        queue_count = gpu->queue_family_properties().size();
            for (uint32_t queue_idx   = 0; static_cast<size_t>(queue_idx) < queue_count; queue_idx++) {
                if (gpu->handle().getSurfaceSupportKHR(queue_idx, surface)) {
                    return *gpu;
                }
            }
        }
    }

    LOGW("Couldn't find a discrete physical device, picking default GPU");
    return *gpus[0];
}

void vk_instance::query_gpus()
{
    std::vector<vk::PhysicalDevice> physical_devices = handle_.enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        throw std::runtime_error("Couldn't find a physical device that supports Vulkan.");
    }

    for (auto& physical_device: physical_devices) {
        gpus.push_back(std::make_unique<vk_physical_device>(*this, physical_device));
    }
}