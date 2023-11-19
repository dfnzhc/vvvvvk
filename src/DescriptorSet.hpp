/**
 * @File DescriptorSet.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_descriptor_set_layout;

class vk_descriptor_pool;

class vk_descriptor_set
{
public:
    vk_descriptor_set(vk_device& device,
                      const vk_descriptor_set_layout& descriptor_set_layout,
                      vk_descriptor_pool& descriptor_pool,
                      const BindingMap<VkDescriptorBufferInfo>& buffer_infos = {},
                      const BindingMap<VkDescriptorImageInfo>& image_infos = {});

    vk_descriptor_set(const vk_descriptor_set&) = delete;

    vk_descriptor_set(vk_descriptor_set&& other);

    // The descriptor set handle is managed by the pool, and will be destroyed when the pool is reset
    ~vk_descriptor_set() = default;

    vk_descriptor_set& operator=(const vk_descriptor_set&) = delete;

    vk_descriptor_set& operator=(vk_descriptor_set&&) = delete;

    /**
     * @brief Resets the DescriptorSet state
     *        Optionally prepares a new set of buffer infos and/or image infos
     * @param new_buffer_infos A map of buffer descriptors and their respective bindings
     * @param new_image_infos A map of image descriptors and their respective bindings
     */
    void reset(const BindingMap<VkDescriptorBufferInfo>& new_buffer_infos = {},
               const BindingMap<VkDescriptorImageInfo>& new_image_infos = {});

    /**
     * @brief Updates the contents of the DescriptorSet by performing the write operations
     * @param bindings_to_update If empty. we update all bindings. Otherwise, only write the specified bindings if they haven't already been written
     */
    void update(const std::vector<uint32_t>& bindings_to_update = {});

    /**
     * @brief Applies pending write operations without updating the state
     */
    void apply_writes() const;

    const vk_descriptor_set_layout& get_layout() const;

    VkDescriptorSet get_handle() const;

    BindingMap<VkDescriptorBufferInfo>& get_buffer_infos();

    BindingMap<VkDescriptorImageInfo>& get_image_infos();

protected:
    /**
     * @brief Prepares the descriptor set to have its contents updated by loading a vector of write operations
     *        Cannot be called twice during the lifetime of a DescriptorSet
     */
    void prepare();

private:
    vk_device& device;

    const vk_descriptor_set_layout& descriptor_set_layout;

    vk_descriptor_pool& descriptor_pool;

    BindingMap<VkDescriptorBufferInfo> buffer_infos;

    BindingMap<VkDescriptorImageInfo> image_infos;

    VkDescriptorSet handle{VK_NULL_HANDLE};

    // The list of write operations for the descriptor set
    std::vector<VkWriteDescriptorSet> write_descriptor_sets;

    // The bindings of the write descriptors that have had vkUpdateDescriptorSets since the last call to update().
    // Each binding number is mapped to a hash of the binding description that it will be updated to.
    std::unordered_map<uint32_t, size_t> updated_bindings;
};