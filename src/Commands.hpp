/**
 * @File Commands.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/23
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class CommandPool
{
public:
    CommandPool() = default;

    ~CommandPool() { deinit(); }

    CommandPool(CommandPool const&) = delete;
    CommandPool& operator=(CommandPool const&) = delete;

    // 如果默认队列为空，使用族索引的第一个队列
    CommandPool(vk::Device device,
                uint32_t familyIndex,
                vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient,
                vk::Queue defaultQueue = VK_NULL_HANDLE)
    {
        init(device, familyIndex, flags, defaultQueue);
    }

    // 如果默认队列为空，使用族索引的第一个队列
    void init(vk::Device device,
              uint32_t familyIndex,
              vk::CommandPoolCreateFlags flags = vk::CommandPoolCreateFlagBits::eTransient,
              vk::Queue defaultQueue = VK_NULL_HANDLE);

    void deinit();

    vk::CommandBuffer createCommandBuffer(vk::CommandBufferLevel level = vk::CommandBufferLevel::ePrimary,
                                          bool begin = true,
                                          vk::CommandBufferUsageFlags flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit,
                                          const vk::CommandBufferInheritanceInfo* pInheritanceInfo = nullptr);

    // 释放这个池中分配的命令缓冲区
    void free(size_t count, const vk::CommandBuffer* cmds);

    void free(const std::vector<vk::CommandBuffer>& cmds) { free(cmds.size(), cmds.data()); }

    void free(vk::CommandBuffer cmd) { free(1, &cmd); }

    vk::CommandPool commandPool() const { return _commandPool; }

    // 结束命令缓冲区的录制并提交到队列
    // 1. 如果 fence 不为空，那么会用作等待缓冲区执行完毕
    // 2. 提交并不会释放缓冲区
    void submit(size_t count, const vk::CommandBuffer* cmds, vk::Queue queue, vk::Fence fence = VK_NULL_HANDLE);
    void submit(size_t count, const vk::CommandBuffer* cmds, vk::Fence fence = VK_NULL_HANDLE);
    void submit(const std::vector<vk::CommandBuffer>& cmds, vk::Fence fence = VK_NULL_HANDLE);

    // @formatter:off
    // 结束命令缓冲区的录制、提交到队列并等待队列结束，之后释放命令缓冲区
    void submitAndWait(size_t count, const vk::CommandBuffer* cmds, vk::Queue queue);
    void submitAndWait(const std::vector<vk::CommandBuffer>& cmds, vk::Queue queue) { submitAndWait(cmds.size(), cmds.data(), queue); }
    void submitAndWait(vk::CommandBuffer cmd, vk::Queue queue) { submitAndWait(1, &cmd, queue); }

    // 结束并提交到默认的队列中，之后会等待队列并释放命令缓冲区
    void submitAndWait(size_t count, const vk::CommandBuffer* cmds) { submitAndWait(count, cmds, _queue); }
    void submitAndWait(const std::vector<vk::CommandBuffer>& cmds) { submitAndWait(cmds.size(), cmds.data(), _queue); }
    void submitAndWait(vk::CommandBuffer cmd) { submitAndWait(1, &cmd, _queue); }
    // @formatter:on

protected:
    vk::Device      _device      = VK_NULL_HANDLE;
    vk::Queue       _queue       = VK_NULL_HANDLE;
    vk::CommandPool _commandPool = VK_NULL_HANDLE;
};

class ScopeCommandBuffer : public CommandPool
{
public:
    ScopeCommandBuffer(VkDevice device, uint32_t familyIndex, VkQueue queue = VK_NULL_HANDLE)
    {
        CommandPool::init(device, familyIndex, vk::CommandPoolCreateFlagBits::eTransient, queue);
        m_cmd = createCommandBuffer();
    }

    ~ScopeCommandBuffer() { submitAndWait(m_cmd); }

    explicit operator vk::CommandBuffer() const { return m_cmd; };

private:
    vk::CommandBuffer m_cmd;
};

class vk_buffer;
class vk_image;
class vk_image_view;

// @formatter:off
void set_viewport(vk::CommandBuffer cmd_buf, uint32_t first_viewport, const std::vector<vk::Viewport> &viewports);
void set_scissor(vk::CommandBuffer cmd_buf, uint32_t first_scissor, const std::vector<vk::Rect2D> &scissors);
void set_line_width(vk::CommandBuffer cmd_buf, float line_width);
void set_depth_bias(vk::CommandBuffer cmd_buf, float depth_bias_constant_factor, float depth_bias_clamp, float depth_bias_slope_factor);
void set_blend_constants(vk::CommandBuffer cmd_buf, const std::array<float, 4> &blend_constants);
void set_depth_bounds(vk::CommandBuffer cmd_buf, float min_depth_bounds, float max_depth_bounds);
void update_buffer(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, vk::DeviceSize offset, const std::vector<uint8_t>& data);
// @formatter:on

void copy_buffer(vk::CommandBuffer cmd_buf, const vk_buffer& src_buffer, const vk_buffer& dst_buffer, vk::DeviceSize size = 0);
void copy_image(vk::CommandBuffer cmd_buf, const vk_image& src_img, const vk_image& dst_img, const std::vector<vk::ImageCopy>& regions);

// @formatter:off
void copy_buffer_to_image(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, const vk_image& image, const std::vector<vk::BufferImageCopy>& regions);
void copy_image_to_buffer(vk::CommandBuffer cmd_buf, const vk_image& image, vk::ImageLayout image_layout, const vk_buffer& buffer, const std::vector<vk::BufferImageCopy>& regions);

void image_memory_barrier(vk::CommandBuffer cmd_buf, const vk_image_view& image_view, const ImageMemoryBarrier& memory_barrier);
void buffer_memory_barrier(vk::CommandBuffer cmd_buf, const vk_buffer& buffer, vk::DeviceSize offset, vk::DeviceSize size, const BufferMemoryBarrier& memory_barrier);
// @formatter:on