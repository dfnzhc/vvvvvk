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

    //--------------------------------------------------------------------------------------------------
    // 创建缓冲区

    std::unique_ptr<vk_buffer> createBuffer(const vk::DeviceSize& size,
                                            const void* data,
                                            vk::BufferUsageFlags usage,
                                            vk::MemoryPropertyFlags memProps = vk::MemoryPropertyFlagBits::eDeviceLocal);

    template<typename T>
    std::unique_ptr<vk_buffer> createBuffer(const std::vector<T>& data,
                                            vk::BufferUsageFlags usage,
                                            vk::MemoryPropertyFlags memProps_ = vk::MemoryPropertyFlagBits::eDeviceLocal)
    {
        return createBuffer(sizeof(T) * data.size(), data.data(), usage, memProps_);
    }
    
    vk::CommandBuffer beginSingleTimeCommands();

    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);


    vk_fence_pool& get_fence_pool();

private:
    const vk_physical_device& gpu;

    vk::SurfaceKHR surface{nullptr};

    std::unique_ptr<vk_debug_utils> debug_utils;

    std::vector<vk::ExtensionProperties> device_extensions;

    std::vector<const char*> enabled_extensions{};

    VmaAllocator memory_allocator{VK_NULL_HANDLE};

    const vk_queue* queue_;
    std::vector<std::vector<vk_queue>> queues;
    
    vk::CommandPool commandPool{VK_NULL_HANDLE};

    std::unique_ptr<vk_fence_pool> fence_pool;
};