#pragma once

#include <core/hpp_instance.h>
#include <map>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class instance;

        struct DriverVersion
        {
            uint16_t major;
            uint16_t minor;
            uint16_t patch;
        };

        class physical_device
        {
        public:
            physical_device(instance& instance, vk::PhysicalDevice physical_device);

            physical_device(const physical_device&) = delete;
            physical_device(physical_device&&) = delete;

            physical_device& operator=(const physical_device&) = delete;
            physical_device& operator=(physical_device&&) = delete;

            DriverVersion get_driver_version() const;
            void* get_extension_feature_chain() const;

            const vk::PhysicalDeviceFeatures& get_features() const;

            vk::PhysicalDevice get_handle() const;
            vkb::core::instance& get_instance() const;

            const vk::PhysicalDeviceMemoryProperties& get_memory_properties() const;

            uint32_t
            get_memory_type(uint32_t bits, vk::MemoryPropertyFlags properties, vk::Bool32* memory_type_found = nullptr) const;

            const vk::PhysicalDeviceProperties& get_properties() const;

            const std::vector<vk::QueueFamilyProperties>& get_queue_family_properties() const;

            const vk::PhysicalDeviceFeatures get_requested_features() const;

            vk::PhysicalDeviceFeatures& get_mutable_requested_features();

            template <typename HPPStructureType>
            HPPStructureType& request_extension_features()
            {
                if (!instance_.is_enabled(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME))
                {
                    throw std::runtime_error("Couldn't request feature from device as " +
                        std::string(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) +
                        " isn't enabled!");
                }

                vk::StructureType structureType = HPPStructureType::structureType; // need to instantiate this value to be usable in find()!

                auto extension_features_it = extension_features_.find(structureType);
                if (extension_features_it != extension_features_.end())
                {
                    return *static_cast<HPPStructureType*>(extension_features_it->second.get());
                }

                vk::StructureChain<vk::PhysicalDeviceFeatures2KHR, HPPStructureType> featureChain = handle_.getFeatures2KHR<vk::PhysicalDeviceFeatures2KHR, HPPStructureType>();

                extension_features_.insert(
                    {structureType, std::make_shared<HPPStructureType>(featureChain.template get<HPPStructureType>())});

                auto* extension_ptr = static_cast<HPPStructureType*>(extension_features_.find(structureType)->second.get());
                if (last_requested_extension_feature_)
                {
                    extension_ptr->pNext = last_requested_extension_feature_;
                }
                last_requested_extension_feature_ = extension_ptr;

                return *extension_ptr;
            }

            void set_high_priority_graphics_queue_enable(bool enable)
            {
                high_priority_graphics_queue_ = enable;
            }

            bool has_high_priority_graphics_queue() const
            {
                return high_priority_graphics_queue_;
            }

        private:
            instance& instance_;

            vk::PhysicalDevice handle_{nullptr};

            vk::PhysicalDeviceFeatures features_;
            vk::PhysicalDeviceProperties properties_;
            vk::PhysicalDeviceMemoryProperties memory_properties_;
            std::vector<vk::QueueFamilyProperties> queue_family_properties_;
            
            vk::PhysicalDeviceFeatures requested_features_;
            void* last_requested_extension_feature_{nullptr};
            std::map<vk::StructureType, std::shared_ptr<void>> extension_features_;
            
            bool high_priority_graphics_queue_{false};
        };
    } // namespace core
} // namespace vkb
