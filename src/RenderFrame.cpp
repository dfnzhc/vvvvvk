/**
 * @File vk_render_frame.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "RenderFrame.hpp"
#include "CommandBufferPool.hpp"
#include "Device.hpp"
#include "RenderTarget.hpp"
#include "ImageView.hpp"
#include "ResourceCaching.hpp"

vk_render_frame::vk_render_frame(vk_device& device, std::unique_ptr<vk_render_target>&& render_target,
                                 size_t thread_count) :
    device{device},
    fence_pool{device},
    semaphore_pool{device},
    swapchain_render_target{std::move(render_target)},
    thread_count{thread_count}
{
    for (auto& usage_it: supported_usage_map) {
        std::vector<std::pair<vk_buffer_pool, vk_buffer_block*>> usage_buffer_pools;
        for (size_t                                              i = 0; i < thread_count; ++i) {
            usage_buffer_pools.emplace_back(
                vk_buffer_pool{device,
                               vk::DeviceSize(BUFFER_POOL_BLOCK_SIZE * 1024 * usage_it.second),
                               vk::BufferUsageFlagBits(usage_it.first)},
                nullptr);
        }

        auto res_ins_it = buffer_pools.emplace(usage_it.first, std::move(usage_buffer_pools));

        if (!res_ins_it.second) {
            throw std::runtime_error("Failed to insert buffer pool");
        }
    }

    for (size_t i = 0; i < thread_count; ++i) {
        descriptor_pools.push_back(std::make_unique<std::unordered_map<std::size_t, vk_descriptor_pool>>());
        descriptor_sets.push_back(std::make_unique<std::unordered_map<std::size_t, vk_descriptor_set>>());
    }
}

vk_device& vk_render_frame::get_device()
{
    return device;
}

void vk_render_frame::update_render_target(std::unique_ptr<vk_render_target>&& render_target)
{
    swapchain_render_target = std::move(render_target);
}

void vk_render_frame::reset()
{
    VK_CHECK(fence_pool.wait());

    fence_pool.reset();

    for (auto& command_pools_per_queue: command_pools) {
        for (auto& command_pool: command_pools_per_queue.second) {
            command_pool->reset_pool();
        }
    }

    for (auto& buffer_pools_per_usage: buffer_pools) {
        for (auto& buffer_pool: buffer_pools_per_usage.second) {
            buffer_pool.first.reset();
            buffer_pool.second = nullptr;
        }
    }

    semaphore_pool.reset();

    if (descriptor_management_strategy == DescriptorManagementStrategy::CreateDirectly) {
        clear_descriptors();
    }
}

std::vector<std::unique_ptr<vk_command_pool>>&
vk_render_frame::get_command_pools(const vk_queue& queue, vk_command_buffer::reset_mode reset_mode)
{
    auto command_pool_it = command_pools.find(queue.get_family_index());

    if (command_pool_it != command_pools.end()) {
        assert(!command_pool_it->second.empty());
        if (command_pool_it->second[0]->get_reset_mode() != reset_mode) {
            device.wait_idle();

            command_pools.erase(command_pool_it);
        } else {
            return command_pool_it->second;
        }
    }

    std::vector<std::unique_ptr<vk_command_pool>> queue_command_pools;

    for (size_t i = 0; i < thread_count; i++) {
        queue_command_pools.push_back(
            std::make_unique<vk_command_pool>(device, queue.get_family_index(), this, i, reset_mode));
    }

    auto res_ins_it = command_pools.emplace(queue.get_family_index(), std::move(queue_command_pools));

    if (!res_ins_it.second) {
        throw std::runtime_error("Failed to insert command pool");
    }

    command_pool_it = res_ins_it.first;

    return command_pool_it->second;
}

std::vector<uint32_t> vk_render_frame::collect_bindings_to_update(const vk_descriptor_set_layout& descriptor_set_layout,
                                                                  const BindingMap<VkDescriptorBufferInfo>& buffer_infos,
                                                                  const BindingMap<VkDescriptorImageInfo>& image_infos)
{
    std::vector<uint32_t> bindings_to_update;

    bindings_to_update.reserve(buffer_infos.size() + image_infos.size());
    auto aggregate_binding_to_update = [&bindings_to_update, &descriptor_set_layout](const auto& infos_map) {
        for (const auto& pair: infos_map) {
            uint32_t binding_index = pair.first;
            if (!(descriptor_set_layout.get_layout_binding_flag(binding_index) &
                  VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT) &&
                std::find(bindings_to_update.begin(), bindings_to_update.end(), binding_index) ==
                bindings_to_update.end()) {
                bindings_to_update.push_back(binding_index);
            }
        }
    };
    aggregate_binding_to_update(buffer_infos);
    aggregate_binding_to_update(image_infos);

    return bindings_to_update;
}

const vk_fence_pool& vk_render_frame::get_fence_pool() const
{
    return fence_pool;
}

VkFence vk_render_frame::request_fence()
{
    return fence_pool.request_fence();
}

const vk_semaphore_pool& vk_render_frame::get_semaphore_pool() const
{
    return semaphore_pool;
}

VkSemaphore vk_render_frame::request_semaphore()
{
    return semaphore_pool.request_semaphore();
}

VkSemaphore vk_render_frame::request_semaphore_with_ownership()
{
    return semaphore_pool.request_semaphore_with_ownership();
}

void vk_render_frame::release_owned_semaphore(VkSemaphore semaphore)
{
    semaphore_pool.release_owned_semaphore(semaphore);
}

vk_render_target& vk_render_frame::get_render_target()
{
    return *swapchain_render_target;
}

const vk_render_target& vk_render_frame::get_render_target_const() const
{
    return *swapchain_render_target;
}

vk_command_buffer&
vk_render_frame::request_command_buffer(const vk_queue& queue, vk_command_buffer::reset_mode reset_mode,
                                        VkCommandBufferLevel level, size_t thread_index)
{
    assert(thread_index < thread_count && "Thread index is out of bounds");

    auto& command_pools = get_command_pools(queue, reset_mode);
    auto command_pool_it = std::find_if(command_pools.begin(), command_pools.end(),
                                        [&thread_index](std::unique_ptr<vk_command_pool>& cmd_pool) {
                                            return cmd_pool->get_thread_index() == thread_index;
                                        });
    
    return (*command_pool_it)->request_command_buffer(vk::CommandBufferLevel(level));
}

VkDescriptorSet vk_render_frame::request_descriptor_set(const vk_descriptor_set_layout& descriptor_set_layout,
                                                        const BindingMap<VkDescriptorBufferInfo>& buffer_infos,
                                                        const BindingMap<VkDescriptorImageInfo>& image_infos,
                                                        bool update_after_bind, size_t thread_index)
{
    assert(thread_index < thread_count && "Thread index is out of bounds");
    assert(thread_index < descriptor_pools.size());

    auto& descriptor_pool = request_resource(device, *descriptor_pools[thread_index], descriptor_set_layout);
    if (descriptor_management_strategy == DescriptorManagementStrategy::StoreInCache) {
        // The bindings we want to update before binding, if empty we update all bindings
        std::vector<uint32_t> bindings_to_update;
        // If update after bind is enabled, we store the binding index of each binding that need to be updated before being bound
        if (update_after_bind) {
            bindings_to_update = collect_bindings_to_update(descriptor_set_layout, buffer_infos, image_infos);
        }

        // Request a descriptor set from the render frame, and write the buffer infos and image infos of all the specified bindings
        assert(thread_index < descriptor_sets.size());
        auto& descriptor_set = request_resource(device, *descriptor_sets[thread_index], descriptor_set_layout,
                                                descriptor_pool, buffer_infos, image_infos);
        descriptor_set.update(bindings_to_update);
        return descriptor_set.get_handle();
    } else {
        // Request a descriptor pool, allocate a descriptor set, write buffer and image data to it
        vk_descriptor_set descriptor_set{device, descriptor_set_layout, descriptor_pool, buffer_infos, image_infos};
        descriptor_set.apply_writes();
        return descriptor_set.get_handle();
    }
}

void vk_render_frame::update_descriptor_sets(size_t thread_index)
{
    assert(thread_index < descriptor_sets.size());
    auto     & thread_descriptor_sets = *descriptor_sets[thread_index];
    for (auto& descriptor_set_it: thread_descriptor_sets) {
        descriptor_set_it.second.update();
    }
}

void vk_render_frame::clear_descriptors()
{
    for (auto& desc_sets_per_thread: descriptor_sets) {
        desc_sets_per_thread->clear();
    }

    for (auto& desc_pools_per_thread: descriptor_pools) {
        for (auto& desc_pool: *desc_pools_per_thread) {
            desc_pool.second.reset();
        }
    }
}

void vk_render_frame::set_buffer_allocation_strategy(BufferAllocationStrategy new_strategy)
{
    buffer_allocation_strategy = new_strategy;
}

void vk_render_frame::set_descriptor_management_strategy(DescriptorManagementStrategy new_strategy)
{
    descriptor_management_strategy = new_strategy;
}

vk_buffer_allocation
vk_render_frame::allocate_buffer(const VkBufferUsageFlags usage, const VkDeviceSize size, size_t thread_index)
{
    assert(thread_index < thread_count && "Thread index is out of bounds");

    // Find a pool for this usage
    auto buffer_pool_it = buffer_pools.find(usage);
    if (buffer_pool_it == buffer_pools.end()) {
        LOGE("No buffer pool for buffer usage {}", usage);
        return vk_buffer_allocation{};
    }

    assert(thread_index < buffer_pool_it->second.size());
    auto& buffer_pool  = buffer_pool_it->second[thread_index].first;
    auto& buffer_block = buffer_pool_it->second[thread_index].second;

    bool want_minimal_block = buffer_allocation_strategy == BufferAllocationStrategy::OneAllocationPerBuffer;

    if (want_minimal_block || !buffer_block || !buffer_block->can_allocate(size)) {
        // If we are creating a buffer for each allocation of there is no block associated with the pool or the current block is too small
        // for this allocation, request a new buffer block
        buffer_block = &buffer_pool.request_buffer_block(size, want_minimal_block);
    }

    return buffer_block->allocate(to_u32(size));
}