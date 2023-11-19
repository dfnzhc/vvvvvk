/**
 * @File vk_render_frame.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include <map>
#include "VkCommon.hpp"
#include "FencePool.hpp"
#include "SemaphorePool.hpp"
#include "BufferPool.hpp"
#include "Queue.hpp"
#include "CommandBuffer.hpp"
#include "DescriptorSetLayout.hpp"
#include "DescriptorPool.hpp"
#include "DescriptorSet.hpp"

class vk_device;
class vk_render_target;

enum BufferAllocationStrategy
{
    OneAllocationPerBuffer,
    MultipleAllocationsPerBuffer
};

enum DescriptorManagementStrategy
{
    StoreInCache,
    CreateDirectly
};

class vk_render_frame
{
public:
    /**
	 * @brief 缓冲区池的块大小(kilobytes)
	 */
    static constexpr uint32_t BUFFER_POOL_BLOCK_SIZE = 256;

    // 所支持的缓冲区用途对 BUFFER_POOL_BLOCK_SIZE 的乘数
    const std::unordered_map<VkBufferUsageFlags, uint32_t> supported_usage_map = {
        {VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 1},
        {VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, 2},        // 之所以乘2，因为 SSBOs 通常比其他类型缓冲区大
        {VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,  1},
        {VK_BUFFER_USAGE_INDEX_BUFFER_BIT,   1}};

    vk_render_frame(vk_device& device, std::unique_ptr<vk_render_target>&& render_target, size_t thread_count = 1);

    vk_render_frame(const vk_render_frame&) = delete;
    vk_render_frame(vk_render_frame&&) = delete;

    vk_render_frame& operator=(const vk_render_frame&) = delete;
    vk_render_frame& operator=(vk_render_frame&&) = delete;

    void reset();

    vk_device& get_device();
    const vk_fence_pool& get_fence_pool() const;

    VkFence request_fence();
    const vk_semaphore_pool& get_semaphore_pool() const;

    VkSemaphore request_semaphore();
    VkSemaphore request_semaphore_with_ownership();
    void release_owned_semaphore(VkSemaphore semaphore);

    /**
     * @brief 当交换链变化时调用
     * @param render_target 拥有更新后图像的 render target
     */
    void update_render_target(std::unique_ptr<vk_render_target>&& render_target);

    // @formatter:off
    vk_render_target&       get_render_target();
    const vk_render_target& get_render_target_const() const;
    // @formatter:on

    /**
    * @brief 从池中请求命令缓冲区，请求的帧应该是活动状态
    * @param queue 用于提交命令缓冲区的队列
    * @param reset_mode 用于指示如何使用命令缓冲区，也可能用于池重新创建时设置相应标志
    * @param level 命令缓冲区的等级，primary 或 secondary
    * @param thread_index 指示命令缓冲区池的线程索引
    * @return 当前活动帧的命令缓冲区
    */
    // @formatter:off
    vk_command_buffer& request_command_buffer(const vk_queue& queue,
                                              vk_command_buffer::reset_mode reset_mode = vk_command_buffer::reset_mode::ResetPool,
                                              VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                                              size_t thread_index = 0);
    // @formatter:on


    VkDescriptorSet request_descriptor_set(const vk_descriptor_set_layout& descriptor_set_layout,
                                           const BindingMap<VkDescriptorBufferInfo>& buffer_infos,
                                           const BindingMap<VkDescriptorImageInfo>& image_infos,
                                           bool update_after_bind,
                                           size_t thread_index = 0);

    void clear_descriptors();

    /**
     * @brief 设置新的缓冲区分配策略
     */
    void set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy);

    /**
     * @brief Sets a new descriptor set management strategy
     * @param new_strategy The new descriptor set management strategy
     */
    void set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy);

    /**
     * @param usage 缓冲区的用途
     * @param size 请求的缓冲区大小
     * @param thread_index 指示缓冲区池的线程索引
     * @return 请求的缓冲区分配，可能为空
     */
    vk_buffer_allocation allocate_buffer(VkBufferUsageFlags usage, VkDeviceSize size, size_t thread_index = 0);

    /**
     * @brief Updates all the descriptor sets in the current frame at a specific thread index
     */
    void update_descriptor_sets(size_t thread_index = 0);

private:
    vk_device& device;

    std::vector<std::unique_ptr<vk_command_pool>>&
    get_command_pools(const vk_queue& queue, vk_command_buffer::reset_mode reset_mode);

    /// 当前帧的命令缓冲区池
    std::map<uint32_t, std::vector<std::unique_ptr<vk_command_pool>>> command_pools;

    std::vector<std::unique_ptr<std::unordered_map<std::size_t, vk_descriptor_pool>>> descriptor_pools;
    std::vector<std::unique_ptr<std::unordered_map<std::size_t, vk_descriptor_set>>> descriptor_sets;

    vk_fence_pool fence_pool;

    vk_semaphore_pool semaphore_pool;

    size_t thread_count;

    std::unique_ptr<vk_render_target> swapchain_render_target;

    BufferAllocationStrategy     buffer_allocation_strategy{BufferAllocationStrategy::MultipleAllocationsPerBuffer};
    DescriptorManagementStrategy descriptor_management_strategy{DescriptorManagementStrategy::CreateDirectly};

    std::map<VkBufferUsageFlags, std::vector<std::pair<vk_buffer_pool, vk_buffer_block*>>> buffer_pools;

    static std::vector<uint32_t> collect_bindings_to_update(const vk_descriptor_set_layout& descriptor_set_layout,
                                                            const BindingMap<VkDescriptorBufferInfo>& buffer_infos,
                                                            const BindingMap<VkDescriptorImageInfo>& image_infos);
};