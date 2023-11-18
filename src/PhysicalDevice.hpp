/**
 * @File PhysicalDevice.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include <map>
#include "VkCommon.hpp"
#include "Instance.hpp"

class vk_physical_device
{
public:
    vk_physical_device(vk_instance& instance, vk::PhysicalDevice physical_device);

    vk_physical_device(const vk_physical_device&) = delete;
    vk_physical_device(vk_physical_device&&) = delete;
    vk_physical_device& operator=(const vk_physical_device&) = delete;
    vk_physical_device& operator=(vk_physical_device&&) = delete;

    void* extension_feature_chain() const;

    const vk::PhysicalDeviceFeatures& features() const;

    vk::PhysicalDevice handle() const;

    vk_instance& instance() const;

    const vk::PhysicalDeviceMemoryProperties& memory_properties() const;

    uint32_t memory_type(uint32_t bits,
                         vk::MemoryPropertyFlags properties,
                         vk::Bool32* memory_type_found = nullptr) const;

    const vk::PhysicalDeviceProperties& properties() const;

    const std::vector<vk::QueueFamilyProperties>& queue_family_properties() const;

    const vk::PhysicalDeviceFeatures requested_features() const;

    vk::PhysicalDeviceFeatures& mutable_requested_features();

private:
    vk_instance& instance_;

    vk::PhysicalDevice handle_{nullptr};

    vk::PhysicalDeviceFeatures features_;

    vk::PhysicalDeviceProperties properties_;

    vk::PhysicalDeviceMemoryProperties memory_properties_;

    std::vector<vk::QueueFamilyProperties> queue_family_properties_;

    vk::PhysicalDeviceFeatures requested_features_;

    void* last_requested_extension_feature_{nullptr};
    std::map<vk::StructureType, std::shared_ptr<void>> extension_features_;
};