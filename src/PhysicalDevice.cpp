/**
 * @File PhysicalDevice.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "PhysicalDevice.hpp"

vk_physical_device::vk_physical_device(vk_instance& instance, vk::PhysicalDevice physical_device) :
    instance_{instance},
    handle_{physical_device}
{
    features_          = physical_device.getFeatures();
    properties_        = physical_device.getProperties();
    memory_properties_ = physical_device.getMemoryProperties();

    LOGI("Found GPU: {}", properties_.deviceName.data());

    queue_family_properties_ = physical_device.getQueueFamilyProperties();
}

void* vk_physical_device::extension_feature_chain() const
{
    return last_requested_extension_feature_;
}

const vk::PhysicalDeviceFeatures& vk_physical_device::features() const
{
    return features_;
}

vk::PhysicalDevice vk_physical_device::handle() const
{
    return handle_;
}

vk_instance& vk_physical_device::instance() const
{
    return instance_;
}

const vk::PhysicalDeviceMemoryProperties& vk_physical_device::memory_properties() const
{
    return memory_properties_;
}

uint32_t vk_physical_device::memory_type(uint32_t bits, vk::MemoryPropertyFlags properties,
                                         vk::Bool32* memory_type_found) const
{
    for (uint32_t i = 0; i < memory_properties_.memoryTypeCount; i++) {
        if ((bits & 1) == 1) {
            if ((memory_properties_.memoryTypes[i].propertyFlags & properties) == properties) {
                if (memory_type_found) {
                    *memory_type_found = true;
                }
                return i;
            }
        }
        bits >>= 1;
    }

    if (memory_type_found) {
        *memory_type_found = false;
        return ~0;
    } else {
        throw std::runtime_error("Could not find a matching memory type");
    }
}

const vk::PhysicalDeviceProperties& vk_physical_device::properties() const
{
    return properties_;
}

const std::vector<vk::QueueFamilyProperties>& vk_physical_device::queue_family_properties() const
{
    return queue_family_properties_;
}

const vk::PhysicalDeviceFeatures vk_physical_device::requested_features() const
{
    return requested_features_;
}

vk::PhysicalDeviceFeatures& vk_physical_device::mutable_requested_features()
{
    return requested_features_;
}
