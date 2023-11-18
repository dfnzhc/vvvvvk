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

    template<typename HPPStructureType>
    HPPStructureType& request_extension_features()
    {
        if (!instance_.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME)) {
            throw std::runtime_error("Couldn't request feature from device as " +
                                     std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
                                     " isn't enabled!");
        }

        vk::StructureType structureType = HPPStructureType::structureType;
        
        auto extension_features_it = extension_features.find(structureType);
        if (extension_features_it != extension_features.end()) {
            return *static_cast<HPPStructureType*>(extension_features_it->second.get());
        }

        auto featureChain = handle_.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, HPPStructureType>();

        extension_features.insert(
            {structureType, std::make_shared<HPPStructureType>(featureChain.template get<HPPStructureType>())});

        auto* extension_ptr = static_cast<HPPStructureType*>(extension_features.find(structureType)->second.get());

        if (last_requested_extension_feature) {
            extension_ptr->pNext = last_requested_extension_feature;
        }
        last_requested_extension_feature = extension_ptr;

        return *extension_ptr;
    }

    void* get_extension_feature_chain() const { return last_requested_extension_feature; }
    
    const vk::PhysicalDeviceFeatures get_requested_features() const { return requested_features; }

    vk::PhysicalDeviceFeatures &get_mutable_requested_features() { return requested_features; }

private:
    const vk_instance& instance_;

    vk::PhysicalDevice handle_{nullptr};

    vk::PhysicalDeviceFeatures         features_;
    vk::PhysicalDeviceProperties       properties_;
    vk::PhysicalDeviceMemoryProperties memory_properties_;

    std::vector<vk::QueueFamilyProperties> queue_family_properties_;
    vk::SampleCountFlagBits                msaaSamples_ = vk::SampleCountFlagBits::e1;

    vk::PhysicalDeviceFeatures requested_features;
    
    void* last_requested_extension_feature{nullptr};
    std::map<vk::StructureType, std::shared_ptr<void>> extension_features;
};