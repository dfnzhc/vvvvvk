/**
 * @File BufferPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "BufferPool.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"
#include "Helpers.hpp"

vk_buffer_block::vk_buffer_block(vk_device& device, vk::DeviceSize size, vk::BufferUsageFlags usage,
                                 VmaMemoryUsage memory_usage) :
    buffer{device, size, usage, memory_usage}
{
    if (usage == vk::BufferUsageFlagBits::eUniformBuffer) {
        alignment = device.get_gpu().properties().limits.minUniformBufferOffsetAlignment;
    } else if (usage == vk::BufferUsageFlagBits::eStorageBuffer) {
        alignment = device.get_gpu().properties().limits.minStorageBufferOffsetAlignment;
    } else if (usage == vk::BufferUsageFlagBits::eUniformTexelBuffer) {
        alignment = device.get_gpu().properties().limits.minTexelBufferOffsetAlignment;
    } else if (usage == vk::BufferUsageFlagBits::eIndexBuffer ||
               usage == vk::BufferUsageFlagBits::eVertexBuffer ||
               usage == vk::BufferUsageFlagBits::eIndirectBuffer) {
        alignment = 16;
    } else {
        throw std::runtime_error("未知的用途");
    }
}

vk::DeviceSize vk_buffer_block::aligned_offset() const
{
    return (offset + alignment - 1) & ~(alignment - 1);
}

bool vk_buffer_block::can_allocate(vk::DeviceSize size) const
{
    assert(size > 0 && "Allocation size must be greater than zero");
    return (aligned_offset() + size <= buffer.get_size());
}

vk_buffer_allocation vk_buffer_block::allocate(vk::DeviceSize size)
{
    if (can_allocate(size)) {
        auto aligned = aligned_offset();
        offset = aligned + size;
        return vk_buffer_allocation{buffer, size, aligned};
    }

    return vk_buffer_allocation{};
}

vk::DeviceSize vk_buffer_block::get_size() const
{
    return buffer.get_size();
}

void vk_buffer_block::reset()
{
    offset = 0;
}

vk_buffer_pool::vk_buffer_pool(vk_device& device, vk::DeviceSize block_size, vk::BufferUsageFlags usage,
                               VmaMemoryUsage memory_usage) :
    device{device},
    block_size{block_size},
    usage{usage},
    memory_usage{memory_usage}
{
}

vk_buffer_block& vk_buffer_pool::request_buffer_block(const vk::DeviceSize minimum_size, bool minimal)
{
    // Find a block in the range of the blocks which can fit the minimum size
    auto it = minimal ? std::find_if(buffer_blocks.begin(), buffer_blocks.end(),
                                     [&minimum_size](const std::unique_ptr<vk_buffer_block>& buffer_block) {
                                         return (buffer_block->get_size() == minimum_size) &&
                                                buffer_block->can_allocate(minimum_size);
                                     }) :

              std::find_if(buffer_blocks.begin(), buffer_blocks.end(),
                           [&minimum_size](const std::unique_ptr<vk_buffer_block>& buffer_block) {
                               return buffer_block->can_allocate(minimum_size);
                           });

    if (it == buffer_blocks.end()) {
        LOGD("Building #{} buffer block ({})", buffer_blocks.size(), vk::to_string(usage));

        vk::DeviceSize new_block_size = minimal ? minimum_size : std::max(block_size, minimum_size);

        // Create a new block and get the iterator on it
        it = buffer_blocks.emplace(buffer_blocks.end(),
                                   std::make_unique<vk_buffer_block>(device, new_block_size, usage, memory_usage));
    }

    return *it->get();
}

void vk_buffer_pool::reset()
{
    for (auto& buffer_block: buffer_blocks) {
        buffer_block->reset();
    }
}

vk_buffer_allocation::vk_buffer_allocation(vk_buffer& buffer, vk::DeviceSize size, vk::DeviceSize offset) :
    buffer{&buffer},
    size{size},
    base_offset{offset}
{
}

void vk_buffer_allocation::update(const std::vector<uint8_t>& data, uint32_t offset)
{
    assert(buffer && "Invalid buffer pointer");

    if (offset + data.size() <= size) {
        buffer->update(data, to_u32(base_offset) + offset);
    } else {
        LOGE("Ignore buffer allocation update");
    }
}

bool vk_buffer_allocation::empty() const
{
    return size == 0 || buffer == nullptr;
}

vk::DeviceSize vk_buffer_allocation::get_size() const
{
    return size;
}

vk::DeviceSize vk_buffer_allocation::get_offset() const
{
    return base_offset;
}

vk_buffer& vk_buffer_allocation::get_buffer()
{
    assert(buffer && "Invalid buffer pointer");
    return *buffer;
}