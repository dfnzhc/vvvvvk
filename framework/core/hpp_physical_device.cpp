/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <core/hpp_physical_device.h>

#include <common/logging.h>

namespace vkb {
namespace core {
physical_device::physical_device(instance& instance, vk::PhysicalDevice physical_device) :
    instance_{instance},
    handle_{physical_device}
{
    features_          = physical_device.getFeatures();
    properties_        = physical_device.getProperties();
    memory_properties_ = physical_device.getMemoryProperties();

    LOGI("Found GPU: {}", properties_.deviceName.data());

    queue_family_properties_ = physical_device.getQueueFamilyProperties();
}

DriverVersion physical_device::get_driver_version() const
{
    DriverVersion version;

    vk::PhysicalDeviceProperties const& properties = get_properties();
    switch (properties.vendorID) {
        case 0x10DE:
            // Nvidia
            version.major = (properties.driverVersion >> 22) & 0x3ff;
            version.minor = (properties.driverVersion >> 14) & 0x0ff;
            version.patch = (properties.driverVersion >> 6) & 0x0ff;
            // Ignoring optional tertiary info in lower 6 bits
            break;
        case 0x8086:
            version.major = (properties.driverVersion >> 14) & 0x3ffff;
            version.minor = properties.driverVersion & 0x3ffff;
            break;
        default:
            version.major = VK_VERSION_MAJOR(properties.driverVersion);
            version.minor = VK_VERSION_MINOR(properties.driverVersion);
            version.patch = VK_VERSION_PATCH(properties.driverVersion);
            break;
    }

    return version;
}

void* physical_device::get_extension_feature_chain() const
{
    return last_requested_extension_feature_;
}

const vk::PhysicalDeviceFeatures& physical_device::get_features() const
{
    return features_;
}

vk::PhysicalDevice physical_device::get_handle() const
{
    return handle_;
}

vkb::core::instance& physical_device::get_instance() const
{
    return instance_;
}

const vk::PhysicalDeviceMemoryProperties& physical_device::get_memory_properties() const
{
    return memory_properties_;
}

uint32_t physical_device::get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties,
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

const vk::PhysicalDeviceProperties& physical_device::get_properties() const
{
    return properties_;
}

const std::vector<vk::QueueFamilyProperties>& physical_device::get_queue_family_properties() const
{
    return queue_family_properties_;
}

const vk::PhysicalDeviceFeatures physical_device::get_requested_features() const
{
    return requested_features_;
}

vk::PhysicalDeviceFeatures& physical_device::get_mutable_requested_features()
{
    return requested_features_;
}

}        // namespace core
}        // namespace vkb
