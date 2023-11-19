/**
 * @File DescriptorPool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_descriptor_set_layout;

class vk_descriptor_pool
{
public:
    static const uint32_t MAX_SETS_PER_POOL = 16;

    vk_descriptor_pool(vk_device& device,
                       const vk_descriptor_set_layout& descriptor_set_layout,
                       uint32_t pool_size = MAX_SETS_PER_POOL);

    vk_descriptor_pool(const vk_descriptor_pool&) = delete;

    vk_descriptor_pool(vk_descriptor_pool&&) = default;

    ~vk_descriptor_pool();

    vk_descriptor_pool& operator=(const vk_descriptor_pool&) = delete;

    vk_descriptor_pool& operator=(vk_descriptor_pool&&) = delete;

    void reset();

    const vk_descriptor_set_layout& get_descriptor_set_layout() const;

    void set_descriptor_set_layout(const vk_descriptor_set_layout& set_layout);

    VkDescriptorSet allocate();

    VkResult free(VkDescriptorSet descriptor_set);

private:
    vk_device& device;

    const vk_descriptor_set_layout* descriptor_set_layout{nullptr};

    // Descriptor pool size
    std::vector<VkDescriptorPoolSize> pool_sizes;

    // Number of sets to allocate for each pool
    uint32_t pool_max_sets{0};

    // Total descriptor pools created
    std::vector<VkDescriptorPool> pools;

    // Count sets for each pool
    std::vector<uint32_t> pool_sets_count;

    // Current pool index to allocate descriptor set
    uint32_t pool_index{0};

    // Map between descriptor set and pool index
    std::unordered_map<VkDescriptorSet, uint32_t> set_pool_mapping;

    // Find next pool index or create new pool
    uint32_t find_available_pool(uint32_t pool_index);
};