/**
 * @File Debug.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */
 
#include "Debug.hpp"
#include <vulkan/vulkan_core.h>

void debug_utils_ext_debug_utils::set_debug_name(vk::Device device, vk::ObjectType object_type,
                                                 uint64_t object_handle, const char* name) const
{
    vk::DebugUtilsObjectNameInfoEXT name_info(object_type, object_handle, name);
    device.setDebugUtilsObjectNameEXT(name_info);
}

void debug_utils_ext_debug_utils::set_debug_tag(vk::Device device, vk::ObjectType object_type,
                                                uint64_t object_handle, uint64_t tag_name,
                                                const void* tag_data, size_t tag_data_size) const
{
    vk::DebugUtilsObjectTagInfoEXT tag_info(object_type, object_handle, tag_name, tag_data_size, tag_data);
    device.setDebugUtilsObjectTagEXT(tag_info);
}

void debug_marker_ext_debug_utils::set_debug_name(vk::Device device, vk::ObjectType object_type,
                                                  uint64_t object_handle, const char* name) const
{
    vk::DebugMarkerObjectNameInfoEXT name_info(vk::debugReportObjectType(object_type), object_handle, name);
    device.debugMarkerSetObjectNameEXT(name_info);
}

void debug_marker_ext_debug_utils::set_debug_tag(vk::Device device, vk::ObjectType object_type,
                                                 uint64_t object_handle, uint64_t tag_name,
                                                 const void* tag_data, size_t tag_data_size) const
{
    vk::DebugMarkerObjectTagInfoEXT tag_info(vk::debugReportObjectType(object_type), object_handle, tag_name,
                                             tag_data_size, tag_data);
    device.debugMarkerSetObjectTagEXT(tag_info);
}