/**
 * @File DescriptorPool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "DescriptorPool.hpp"
#include "Device.hpp"
#include "DescriptorSetLayout.hpp"

vk_descriptor_pool::vk_descriptor_pool(vk_device& device,
                                       const vk_descriptor_set_layout& descriptor_set_layout,
                                       uint32_t pool_size) :
    device{device},
    descriptor_set_layout{&descriptor_set_layout}
{
    const auto& bindings = descriptor_set_layout.get_bindings();

    std::map<VkDescriptorType, std::uint32_t> descriptor_type_counts;

    // Count each type of descriptor set
    for (auto& binding: bindings) {
        descriptor_type_counts[binding.descriptorType] += binding.descriptorCount;
    }

    // Allocate pool sizes array
    pool_sizes.resize(descriptor_type_counts.size());

    auto pool_size_it = pool_sizes.begin();

    // Fill pool size for each descriptor type count multiplied by the pool size
    for (auto& it: descriptor_type_counts) {
        pool_size_it->type = it.first;

        pool_size_it->descriptorCount = it.second * pool_size;

        ++pool_size_it;
    }

    pool_max_sets = pool_size;
}

vk_descriptor_pool::~vk_descriptor_pool()
{
    // Destroy all descriptor pools
    for (auto pool: pools) {
        vkDestroyDescriptorPool(device.handle(), pool, nullptr);
    }
}

void vk_descriptor_pool::reset()
{
    // Reset all descriptor pools
    for (auto pool: pools) {
        vkResetDescriptorPool(device.handle(), pool, 0);
    }

    // Clear internal tracking of descriptor set allocations
    std::fill(pool_sets_count.begin(), pool_sets_count.end(), 0);
    set_pool_mapping.clear();

    // Reset the pool index from which descriptor sets are allocated
    pool_index = 0;
}

const vk_descriptor_set_layout& vk_descriptor_pool::get_descriptor_set_layout() const
{
    assert(descriptor_set_layout && "Descriptor set layout is invalid");
    return *descriptor_set_layout;
}

void vk_descriptor_pool::set_descriptor_set_layout(const vk_descriptor_set_layout& set_layout)
{
    descriptor_set_layout = &set_layout;
}

VkDescriptorSet vk_descriptor_pool::allocate()
{
    pool_index = find_available_pool(pool_index);

    // Increment allocated set count for the current pool
    ++pool_sets_count[pool_index];

    VkDescriptorSetLayout set_layout = get_descriptor_set_layout().get_handle();

    VkDescriptorSetAllocateInfo alloc_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool     = pools[pool_index];
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts        = &set_layout;

    VkDescriptorSet handle = VK_NULL_HANDLE;

    // Allocate a new descriptor set from the current pool
    auto result = vkAllocateDescriptorSets(device.handle(), &alloc_info, &handle);

    if (result != VK_SUCCESS) {
        // Decrement allocated set count for the current pool
        --pool_sets_count[pool_index];

        return VK_NULL_HANDLE;
    }

    // Store mapping between the descriptor set and the pool
    set_pool_mapping.emplace(handle, pool_index);

    return handle;
}

VkResult vk_descriptor_pool::free(VkDescriptorSet descriptor_set)
{
    // Get the pool index of the descriptor set
    auto it = set_pool_mapping.find(descriptor_set);

    if (it == set_pool_mapping.end()) {
        return VK_INCOMPLETE;
    }

    auto desc_pool_index = it->second;

    // Free descriptor set from the pool
    vkFreeDescriptorSets(device.handle(), pools[desc_pool_index], 1, &descriptor_set);

    // Remove descriptor set mapping to the pool
    set_pool_mapping.erase(it);

    // Decrement allocated set count for the pool
    --pool_sets_count[desc_pool_index];

    // Change the current pool index to use the available pool
    pool_index = desc_pool_index;

    return VK_SUCCESS;
}

std::uint32_t vk_descriptor_pool::find_available_pool(std::uint32_t search_index)
{
    // Create a new pool
    if (pools.size() <= search_index) {
        VkDescriptorPoolCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};

        create_info.poolSizeCount = to_u32(pool_sizes.size());
        create_info.pPoolSizes    = pool_sizes.data();
        create_info.maxSets       = pool_max_sets;

        // We do not set FREE_DESCRIPTOR_SET_BIT as we do not need to free individual descriptor sets
        create_info.flags = 0;

        // Check descriptor set layout and enable the required flags
        auto& binding_flags = descriptor_set_layout->get_binding_flags();
        for (auto binding_flag: binding_flags) {
            if (binding_flag & VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT) {
                create_info.flags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT_EXT;
            }
        }

        VkDescriptorPool handle = VK_NULL_HANDLE;

        // Create the Vulkan descriptor pool
        auto result = vkCreateDescriptorPool(device.handle(), &create_info, nullptr, &handle);

        if (result != VK_SUCCESS) {
            return 0;
        }

        // Store internally the Vulkan handle
        pools.push_back(handle);

        // Add set count for the descriptor pool
        pool_sets_count.push_back(0);

        return search_index;
    } else if (pool_sets_count[search_index] < pool_max_sets) {
        return search_index;
    }

    // Increment pool index
    return find_available_pool(++search_index);
}