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
    explicit vk_physical_device(const vk_instance& instance);

    vk_physical_device(const vk_physical_device&) = delete;
    vk_physical_device(vk_physical_device&&) = delete;
    vk_physical_device& operator=(const vk_physical_device&) = delete;
    vk_physical_device& operator=(vk_physical_device&&) = delete;

    vk::Instance instance() const { return instance_.handle(); }

    vk::PhysicalDevice handle() const { return handle_; }

    const vk::PhysicalDeviceFeatures& features() const { return features_; }

    const vk::PhysicalDeviceMemoryProperties& memory_properties() const { return memory_properties_; }

    uint32_t memory_type(uint32_t bits,
                         vk::MemoryPropertyFlags properties,
                         vk::Bool32* memory_type_found = nullptr) const;

    const vk::PhysicalDeviceProperties& properties() const { return properties_; }

    const std::vector<vk::QueueFamilyProperties>& queue_family_properties() const { return queue_family_properties_; }

    vk::SampleCountFlagBits msaa_samples() const { return msaaSamples_; }

private:
    const vk_instance& instance_;

    vk::PhysicalDevice handle_{nullptr};

    vk::PhysicalDeviceFeatures         features_;
    vk::PhysicalDeviceProperties       properties_;
    vk::PhysicalDeviceMemoryProperties memory_properties_;

    std::vector<vk::QueueFamilyProperties> queue_family_properties_;
    vk::SampleCountFlagBits                msaaSamples_ = vk::SampleCountFlagBits::e1;
};