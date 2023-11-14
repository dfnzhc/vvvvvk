#pragma once

#include <core/hpp_command_buffer.h>
#include <core/hpp_debug.h>
#include <core/hpp_physical_device.h>
#include <core/hpp_queue.h>
#include <core/hpp_vulkan_resource.h>
#include <hpp_fence_pool.h>
#include <hpp_resource_cache.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class HPPBuffer;
        class command_pool;

        class device : public vkb::core::vk_unit<vk::Device>
        {
        public:
            device(vkb::core::physical_device& gpu,
                      vk::SurfaceKHR surface,
                      std::unique_ptr<vkb::core::debug_utils>&& debug_utils,
                      std::unordered_map<const char*, bool> requested_extensions = {});

            ~device();
         
            device(const device&) = delete;
            device(device&&) = delete;

            device& operator=(const device&) = delete;
            device& operator=(device&&) = delete;

            vkb::core::physical_device const& get_gpu() const;
            VmaAllocator const& get_memory_allocator() const;

            vkb::core::debug_utils const& get_debug_utils() const;
         
            vkb::core::queue const& get_queue(uint32_t queue_family_index, uint32_t queue_index) const;
            vkb::core::queue const& get_queue_by_flags(vk::QueueFlags queue_flags, uint32_t queue_index) const;
            vkb::core::queue const& get_queue_by_present(uint32_t queue_index) const;
            vkb::core::queue const& get_suitable_graphics_queue() const;

            uint32_t get_queue_family_index(vk::QueueFlagBits queue_flag) const;
            
            bool is_extension_supported(std::string const& extension) const;
            bool is_enabled(std::string const& extension) const;

            vkb::core::command_pool& get_command_pool();

            std::pair<vk::Buffer, vk::DeviceMemory> create_buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::DeviceSize size, void* data = nullptr) const;
            std::pair<vk::Image, vk::DeviceMemory> create_image(vk::Format format, vk::Extent2D const& extent, uint32_t mip_levels, vk::ImageUsageFlags usage,
                                                                vk::MemoryPropertyFlags properties) const;

            void copy_buffer(vkb::core::HPPBuffer& src, vkb::core::HPPBuffer& dst, vk::Queue queue, vk::BufferCopy* copy_region = nullptr) const;

            vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level, bool begin = false) const;

            void flush_command_buffer(vk::CommandBuffer command_buffer, vk::Queue queue, bool free = true, vk::Semaphore signalSemaphore = VK_NULL_HANDLE) const;

            vkb::fence_pool& get_fence_pool();
            vkb::resource_cache& get_resource_cache();

        private:
            vkb::core::physical_device const& gpu_;

            vk::SurfaceKHR surface_{nullptr};

            std::unique_ptr<vkb::core::debug_utils> debug_utils_;
            
            std::vector<vk::ExtensionProperties> device_extensions_;
            std::vector<const char*> enabled_extensions_{};
            
            VmaAllocator memory_allocator_{VK_NULL_HANDLE};

            std::vector<std::vector<vkb::core::queue>> queues_;

            std::unique_ptr<vkb::core::command_pool> command_pool_;
            std::unique_ptr<vkb::fence_pool> fence_pool_;
            
            vkb::resource_cache resource_cache_;
        };
    } // namespace core
} // namespace vkb
