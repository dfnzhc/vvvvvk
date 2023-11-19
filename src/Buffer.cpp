/**
 * @File Buffer.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "Buffer.hpp"
#include "Device.hpp"

vk_buffer::vk_buffer(vk_device& device,
                     vk::DeviceSize size_,
                     vk::BufferUsageFlags buffer_usage,
                     VmaMemoryUsage memory_usage,
                     VmaAllocationCreateFlags flags,
                     const std::vector<uint32_t>& queue_family_indices) :
    vk_unit(nullptr, &device), size(size_)
{
    persistent = (flags & VMA_ALLOCATION_CREATE_MAPPED_BIT) != 0;

    vk::BufferCreateInfo buffer_create_info({}, size, buffer_usage);
    if (queue_family_indices.size() >= 2) {
        buffer_create_info.sharingMode           = vk::SharingMode::eConcurrent;
        buffer_create_info.queueFamilyIndexCount = static_cast<uint32_t>(queue_family_indices.size());
        buffer_create_info.pQueueFamilyIndices   = queue_family_indices.data();
    }

    VmaAllocationCreateInfo memory_info{};
    memory_info.flags = flags;
    memory_info.usage = memory_usage;

    VmaAllocationInfo allocation_info{};

    auto result = vmaCreateBuffer(device.get_memory_allocator(),
                                  reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info), &memory_info,
                                  reinterpret_cast<VkBuffer*>(&handle()), &allocation,
                                  &allocation_info);

    if (result != VK_SUCCESS) {
        throw VulkanException{vk::Result(result), "创建缓冲区失败"};
    }

    memory = static_cast<vk::DeviceMemory>(allocation_info.deviceMemory);

    if (persistent) {
        mapped_data = static_cast<uint8_t*>(allocation_info.pMappedData);
    }
}

vk_buffer::vk_buffer(vk_buffer&& other) noexcept :
    vk_unit{other.handle(), &other.device()},
    allocation(std::exchange(other.allocation, {})),
    memory(std::exchange(other.memory, {})),
    size(std::exchange(other.size, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    mapped(std::exchange(other.mapped, {})) {}

vk_buffer::~vk_buffer()
{
    if (handle() && (allocation != VK_NULL_HANDLE)) {
        unmap();
        vmaDestroyBuffer(device().get_memory_allocator(), static_cast<VkBuffer>(handle()), allocation);
    }
}

VmaAllocation vk_buffer::get_allocation() const
{
    return allocation;
}

vk::DeviceMemory vk_buffer::get_memory() const
{
    return memory;
}

vk::DeviceSize vk_buffer::get_size() const
{
    return size;
}

const uint8_t* vk_buffer::get_data() const
{
    return mapped_data;
}

uint8_t* vk_buffer::map()
{
    if (!mapped && !mapped_data) {
        VK_CHECK(vmaMapMemory(device().get_memory_allocator(), allocation, reinterpret_cast<void**>(&mapped_data)));
        mapped = true;
    }
    return mapped_data;
}

void vk_buffer::unmap()
{
    if (mapped) {
        vmaUnmapMemory(device().get_memory_allocator(), allocation);
        mapped_data = nullptr;
        mapped      = false;
    }
}

void vk_buffer::flush()
{
    vmaFlushAllocation(device().get_memory_allocator(), allocation, 0, size);
}

void vk_buffer::update(const std::vector<uint8_t>& data, size_t offset)
{
    update(data.data(), data.size(), offset);
}

uint64_t vk_buffer::get_device_address() const
{
    return device().handle().getBufferAddressKHR({handle()});
}

void vk_buffer::update(void* data, size_t size, size_t offset)
{
    update(reinterpret_cast<const uint8_t*>(data), size, offset);
}

void vk_buffer::update(const uint8_t* data, const size_t size, const size_t offset)
{
    if (persistent) {
        std::copy(data, data + size, mapped_data + offset);
        flush();
    } else {
        map();
        std::copy(data, data + size, mapped_data + offset);
        flush();
        unmap();
    }
}