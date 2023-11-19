/**
 * @File DescriptorSetLayout.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "Helpers.hpp"
#include "VkCommon.hpp"

class vk_device;

class ShaderModule;

struct ShaderResource;

class vk_descriptor_set_layout
{
public:
    vk_descriptor_set_layout(vk_device& device,
                             const uint32_t set_index,
                             const std::vector<ShaderModule*>& shader_modules,
                             const std::vector<ShaderResource>& resource_set);

    vk_descriptor_set_layout(vk_descriptor_set_layout&& other);

    ~vk_descriptor_set_layout();

    vk_descriptor_set_layout(const vk_descriptor_set_layout&) = delete;

    vk_descriptor_set_layout& operator=(const vk_descriptor_set_layout&) = delete;
    vk_descriptor_set_layout& operator=(vk_descriptor_set_layout&&) = delete;

    VkDescriptorSetLayout get_handle() const;

    const uint32_t get_index() const;

    const std::vector<VkDescriptorSetLayoutBinding>& get_bindings() const;

    std::unique_ptr<VkDescriptorSetLayoutBinding> get_layout_binding(const uint32_t binding_index) const;

    std::unique_ptr<VkDescriptorSetLayoutBinding> get_layout_binding(const std::string& name) const;

    const std::vector<VkDescriptorBindingFlagsEXT>& get_binding_flags() const;

    VkDescriptorBindingFlagsEXT get_layout_binding_flag(const uint32_t binding_index) const;

    const std::vector<ShaderModule*>& get_shader_modules() const;

private:
    vk_device& device;

    VkDescriptorSetLayout handle{VK_NULL_HANDLE};

    const uint32_t set_index;

    std::vector<VkDescriptorSetLayoutBinding> bindings;

    std::vector<VkDescriptorBindingFlagsEXT> binding_flags;

    std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings_lookup;

    std::unordered_map<uint32_t, VkDescriptorBindingFlagsEXT> binding_flags_lookup;

    std::unordered_map<std::string, uint32_t> resources_lookup;

    std::vector<ShaderModule*> shader_modules;
};