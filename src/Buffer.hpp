/**
 * @File Buffer.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkUnit.hpp"
#include "VkCommon.hpp"

class vk_buffer : public vk_unit<vk::Buffer>
{
public:
    vk_buffer(vk_device& device,
              vk::DeviceSize size,
              vk::BufferUsageFlags buffer_usage,
              VmaMemoryUsage memory_usage,
              VmaAllocationCreateFlags flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
              const std::vector<uint32_t>& queue_family_indices = {});

    vk_buffer(vk_buffer&& other) noexcept;

    ~vk_buffer();

    vk_buffer(const vk_buffer&) = delete;
    vk_buffer& operator=(const vk_buffer&) = delete;
    vk_buffer& operator=(vk_buffer&&) = delete;

    VmaAllocation get_allocation() const;
    const uint8_t* get_data() const;
    vk::DeviceMemory get_memory() const;

    /**
     * @return Return the buffer's device address (note: requires that the buffer has been created with the VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT usage fla)
     */
    uint64_t get_device_address() const;

    /**
     * @return The size of the buffer
     */
    vk::DeviceSize get_size() const;

    /**
     * @brief Flushes memory if it is HOST_VISIBLE and not HOST_COHERENT
     */
    void flush();

    /**
     * @brief Maps vulkan memory if it isn't already mapped to an host visible address
     * @return Pointer to host visible memory
     */
    uint8_t* map();

    /**
     * @brief Unmaps vulkan memory from the host visible address
     */
    void unmap();

    /**
     * @brief Copies byte data into the buffer
     * @param data The data to copy from
     * @param size The amount of bytes to copy
     * @param offset The offset to start the copying into the mapped data
     */
    void update(const uint8_t* data, size_t size, size_t offset = 0);

    /**
     * @brief Converts any non byte data into bytes and then updates the buffer
     * @param data The data to copy from
     * @param size The amount of bytes to copy
     * @param offset The offset to start the copying into the mapped data
     */
    void update(void* data, size_t size, size_t offset = 0);

    /**
     * @brief Copies a vector of bytes into the buffer
     * @param data The data vector to upload
     * @param offset The offset to start the copying into the mapped data
     */
    void update(const std::vector<uint8_t>& data, size_t offset = 0);

    /**
     * @brief Copies an object as byte data into the buffer
     * @param object The object to convert into byte data
     * @param offset The offset to start the copying into the mapped data
     */
    template<class T>
    void convert_and_update(const T& object, size_t offset = 0)
    {
        update(reinterpret_cast<const uint8_t*>(&object), sizeof(T), offset);
    }

private:
    VmaAllocation    allocation    = VK_NULL_HANDLE;
    vk::DeviceMemory memory        = nullptr;
    vk::DeviceSize   size          = 0;
    uint8_t          * mapped_data = nullptr;
    bool             persistent    = false;        // Whether the buffer is persistently mapped or not
    bool             mapped        = false;        // Whether the buffer has been mapped with vmaMapMemory
};