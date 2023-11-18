/**
 * @File PhysicalDevice.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "PhysicalDevice.hpp"

namespace {

uint32_t ScorePhysicalDevice(const vk::PhysicalDevice& device)
{
    uint32_t score = 0;
    const auto& extensionProperties = device.enumerateDeviceExtensionProperties();

    // 根据设备的功能和特性进行打分
    const auto properties = device.getProperties();
    const auto features   = device.getFeatures();

    if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu)
        score += 1000;
    if (properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu)
        score += 500;
    if (properties.deviceType == vk::PhysicalDeviceType::eCpu)
        score += 100;

    score += properties.limits.maxImageDimension2D;

    return score;
}

vk::PhysicalDevice ChoosePhysicalDevice(const std::vector<vk::PhysicalDevice>& devices)
{
    std::multimap<uint32_t, VkPhysicalDevice> rankedDevices;

    auto where = rankedDevices.end();
    for (const auto& device: devices)
        where = rankedDevices.insert(where, {ScorePhysicalDevice(device), device});

    if (rankedDevices.rbegin()->first > 0)
        return rankedDevices.rbegin()->second;

    return VK_NULL_HANDLE;
}

const std::vector<vk::SampleCountFlagBits> STAGE_FLAG_BITS = {
    vk::SampleCountFlagBits::e64, vk::SampleCountFlagBits::e32,
    vk::SampleCountFlagBits::e16, vk::SampleCountFlagBits::e8,
    vk::SampleCountFlagBits::e4, vk::SampleCountFlagBits::e1
};

vk::SampleCountFlagBits GetMaxUsableSampleCount(vk::PhysicalDevice physicalDevice)
{
    const auto& props = physicalDevice.getProperties();

    auto counts = std::min(props.limits.framebufferColorSampleCounts,
                           props.limits.framebufferDepthSampleCounts);

    for (const auto& sampleFlag: STAGE_FLAG_BITS) {
        if (counts & sampleFlag)
            return sampleFlag;
    }

    return vk::SampleCountFlagBits::e1;
}

} // namespace 

vk_physical_device::vk_physical_device(const vk_instance& instance) :
    instance_{instance}
{
    const auto& physical_devices = instance.handle().enumeratePhysicalDevices();
    if (physical_devices.empty()) {
        throw std::runtime_error("没有找到支持 Vulkan 的物理设备");
    }

    handle_ = ChoosePhysicalDevice(physical_devices);
    if (!handle_) {
        throw std::runtime_error("没有找到合适的物理设备");
    }

    features_          = handle_.getFeatures();
    properties_        = handle_.getProperties();
    memory_properties_ = handle_.getMemoryProperties();

    LOGI("Found GPU: {}", properties_.deviceName.data());
    queue_family_properties_ = handle_.getQueueFamilyProperties();

    msaaSamples_ = GetMaxUsableSampleCount(handle_);
}

uint32_t vk_physical_device::memory_type(uint32_t bits,
                                         vk::MemoryPropertyFlags properties,
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
        throw std::runtime_error("没有适合的设备存储类型");
    }
}

