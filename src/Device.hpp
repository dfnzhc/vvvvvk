/**
 * @File Deivce.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"
#include "CommandBuffer.hpp"
#include <vector>

class vk_debug_utils;

class vk_queue;

class vk_physical_device;

class vk_buffer;

class vk_fence_pool;

class vk_device : public vk_unit<vk::Device>
{
public:

    vk_device(vk_physical_device& gpu, vk::SurfaceKHR surface,
              std::unique_ptr<vk_debug_utils>&& debug_utils,
              std::unordered_map<const char*, bool> requested_extensions = {});

    ~vk_device() override;

    vk_device(const vk_device&) = delete;
    vk_device(vk_device&&) = delete;

    vk_device& operator=(const vk_device&) = delete;
    vk_device& operator=(vk_device&&) = delete;

    const vk_physical_device& get_gpu() const;

    const VmaAllocator& get_memory_allocator() const;

    const vk_debug_utils& get_debug_utils() const;

    const vk_queue& get_queue(uint32_t queue_family_index, uint32_t queue_index) const;

    const vk_queue& get_queue_by_flags(vk::QueueFlags queue_flags, uint32_t queue_index) const;

    const vk_queue& get_queue_by_present(uint32_t queue_index) const;

    const vk_queue& get_suitable_graphics_queue() const;

    bool is_extension_supported(const std::string& extension) const;

    bool is_enabled(const std::string& extension) const;

    uint32_t get_queue_family_index(vk::QueueFlagBits queue_flag) const;

    void wait_idle() const;

    std::pair<vk::Buffer, vk::DeviceMemory> create_buffer(vk::BufferUsageFlags usage,
                                                          vk::MemoryPropertyFlags properties,
                                                          vk::DeviceSize size, void* data = nullptr) const;

    std::pair<vk::Image, vk::DeviceMemory> create_image(vk::Format format,
                                                        const vk::Extent2D& extent,
                                                        uint32_t mip_levels,
                                                        vk::ImageUsageFlags usage,
                                                        vk::MemoryPropertyFlags properties) const;

    void copy_buffer(vk_buffer& src, vk_buffer& dst,
                     vk::Queue queue, vk::BufferCopy* copy_region = nullptr) const;


    vk_command_pool& get_command_pool();
    vk::CommandBuffer create_command_buffer(vk::CommandBufferLevel level, bool begin = false) const;
    void flush_command_buffer(vk::CommandBuffer command_buffer,
                              vk::Queue queue, bool free = true, vk::Semaphore signalSemaphore = VK_NULL_HANDLE) const;

    vk_fence_pool& get_fence_pool();

private:
    const vk_physical_device& gpu;

    vk::SurfaceKHR surface{nullptr};

    std::unique_ptr<vk_debug_utils> debug_utils;

    std::vector<vk::ExtensionProperties> device_extensions;

    std::vector<const char*> enabled_extensions{};

    VmaAllocator memory_allocator{VK_NULL_HANDLE};

    std::vector<std::vector<vk_queue>> queues;

    std::unique_ptr<vk_command_pool> command_pool;

    std::unique_ptr<vk_fence_pool> fence_pool;
};