/**
 * @File VkInstance.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "Instance.hpp"
#include <vulkan/vulkan.hpp>

#include "volk.h"

#if (VULKAN_HPP_DISPATCH_LOADER_DYNAMIC == 1)
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#endif

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
    VULKAN_HPP_DEFAULT_DISPATCHER.init(handle_);
    volkLoadInstance(handle_);
    init_debug_utils();
}

vk_instance::~vk_instance()
{
    if (debug_messenger_) {
        handle_.destroyDebugUtilsMessengerEXT(debug_messenger_, nullptr);
        debug_messenger_ = VK_NULL_HANDLE;
    }

    if (handle_) {
        handle_.destroy();
    }

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
    vk::DebugUtilsMessengerCreateInfoEXT dbg_messenger_create_info;
    dbg_messenger_create_info.messageSeverity =
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo       // For debug printf
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning  // GPU info, bug
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError;   // Invalid usage
    dbg_messenger_create_info.messageType     = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral            // Other
                                                |
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation       // Violation of spec
                                                |
                                                vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance;     // Non-optimal use
    dbg_messenger_create_info.pfnUserCallback = debugMessengerCallback;
    dbg_messenger_create_info.pUserData       = this;

    debug_messenger_ = handle_.createDebugUtilsMessengerEXT(dbg_messenger_create_info);
}
