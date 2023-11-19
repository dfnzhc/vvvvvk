/**
 * @File BufferPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "Buffer.hpp"

class vk_device;

class vk_buffer_allocation
{
public:
    vk_buffer_allocation() = default;

    vk_buffer_allocation(vk_buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset);

    vk_buffer_allocation(const vk_buffer_allocation&) = delete;
    vk_buffer_allocation(vk_buffer_allocation&&) = default;

    vk_buffer_allocation& operator=(const vk_buffer_allocation&) = delete;
    vk_buffer_allocation& operator=(vk_buffer_allocation&&) = default;

    void update(const std::vector<uint8_t>& data, uint32_t offset = 0);

    template<class T>
    void update(const T& value, uint32_t offset = 0)
    {
        update(to_bytes(value), offset);
    }

    bool empty() const;

    vk::DeviceSize get_size() const;

    vk::DeviceSize get_offset() const;

    vk_buffer& get_buffer();

private:
    vk_buffer* buffer{nullptr};

    vk::DeviceSize base_offset{0};

    vk::DeviceSize size{0};
};

class vk_buffer_block
{
public:
    vk_buffer_block(vk_device& device, vk::DeviceSize size, vk::BufferUsageFlags usage, VmaMemoryUsage memory_usage);

    bool can_allocate(vk::DeviceSize size) const;

    vk_buffer_allocation allocate(vk::DeviceSize size);

    vk::DeviceSize get_size() const;

    void reset();

private:
    vk::DeviceSize aligned_offset() const;

private:
    vk_buffer      buffer;
    vk::DeviceSize alignment{0};
    vk::DeviceSize offset{0};
};

class vk_buffer_pool
{
public:
    vk_buffer_pool(vk_device& device, vk::DeviceSize block_size, vk::BufferUsageFlags usage,
                   VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU);
    vk_buffer_block& request_buffer_block(vk::DeviceSize minimum_size, bool minimal = false);
    void reset();

private:
    vk_device& device;

    std::vector<std::unique_ptr<vk_buffer_block>> buffer_blocks;

    vk::DeviceSize block_size{0};

    vk::BufferUsageFlags usage{};

    VmaMemoryUsage memory_usage{};
};