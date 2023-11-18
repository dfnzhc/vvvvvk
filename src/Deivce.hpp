/**
 * @File Deivce.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"
#include <vector>

class vk_debug_utils;
class vk_queue;
class vk_physical_device;

class vk_device : public vk_unit<vk::Device>
{
public:

    vk_device(vk_physical_device& gpu, vk::SurfaceKHR surface,
              std::unique_ptr<vk_debug_utils>&& debug_utils,
              std::unordered_map<const char*, bool> requested_extensions = {});

    ~vk_device();

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

private:
    const vk_physical_device& gpu;

    vk::SurfaceKHR surface{nullptr};

    std::unique_ptr<vk_debug_utils> debug_utils;

    std::vector<vk::ExtensionProperties> device_extensions;

    std::vector<const char*> enabled_extensions{};

    VmaAllocator memory_allocator{VK_NULL_HANDLE};

    std::vector<std::vector<vk_queue>> queues;
};