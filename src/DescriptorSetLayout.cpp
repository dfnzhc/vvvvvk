﻿/**
 * @File DescriptorSetLayout.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "DescriptorSetLayout.hpp"
#include "ShaderModule.hpp"
#include "PhysicalDevice.hpp"
#include "Device.hpp"

namespace {
inline VkDescriptorType find_descriptor_type(ShaderResourceType resource_type, bool dynamic)
{
    switch (resource_type) {
        case ShaderResourceType::InputAttachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            break;
        case ShaderResourceType::Image:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        case ShaderResourceType::ImageSampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        case ShaderResourceType::ImageStorage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        case ShaderResourceType::Sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        case ShaderResourceType::BufferUniform:
            if (dynamic) {
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
            } else {
                return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            }
            break;
        case ShaderResourceType::BufferStorage:
            if (dynamic) {
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
            } else {
                return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            }
            break;
        default:
            throw std::runtime_error("No conversion possible for the shader resource type.");
            break;
    }
}

inline bool
validate_binding(const VkDescriptorSetLayoutBinding& binding, const std::vector<VkDescriptorType>& blacklist)
{
    return !(std::find_if(blacklist.begin(), blacklist.end(),
                          [binding](const VkDescriptorType& type) { return type == binding.descriptorType; }) !=
             blacklist.end());
}

inline bool validate_flags(const vk_physical_device& gpu, const std::vector<VkDescriptorSetLayoutBinding>& bindings,
                           const std::vector<VkDescriptorBindingFlagsEXT>& flags)
{
    // Assume bindings are valid if there are no flags
    if (flags.empty()) {
        return true;
    }

    // Binding count has to equal flag count as its a 1:1 mapping
    if (bindings.size() != flags.size()) {
        LOGE("Binding count has to be equal to flag count.");
        return false;
    }

    return true;
}
}        // namespace

vk_descriptor_set_layout::vk_descriptor_set_layout(vk_device& device,
                                                   const uint32_t set_index,
                                                   const std::vector<ShaderModule*>& shader_modules,
                                                   const std::vector<ShaderResource>& resource_set) :
    device{device},
    set_index{set_index},
    shader_modules{shader_modules}
{
    for (auto& resource: resource_set) {
        // Skip shader resources whitout a binding point
        if (resource.type == ShaderResourceType::Input ||
            resource.type == ShaderResourceType::Output ||
            resource.type == ShaderResourceType::PushConstant ||
            resource.type == ShaderResourceType::SpecializationConstant) {
            continue;
        }

        // Convert from ShaderResourceType to VkDescriptorType.
        auto descriptor_type = find_descriptor_type(resource.type, resource.mode == ShaderResourceMode::Dynamic);

        if (resource.mode == ShaderResourceMode::UpdateAfterBind) {
            binding_flags.push_back(VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT);
        } else {
            // When creating a descriptor set layout, if we give a structure to create_info.pNext, each binding needs to have a binding flag
            // (pBindings[i] uses the flags in pBindingFlags[i])
            // Adding 0 ensures the bindings that dont use any flags are mapped correctly.
            binding_flags.push_back(0);
        }

        // Convert ShaderResource to VkDescriptorSetLayoutBinding
        VkDescriptorSetLayoutBinding layout_binding{};

        layout_binding.binding         = resource.binding;
        layout_binding.descriptorCount = resource.array_size;
        layout_binding.descriptorType  = descriptor_type;
        layout_binding.stageFlags      = static_cast<VkShaderStageFlags>(resource.stages);

        bindings.push_back(layout_binding);

        // Store mapping between binding and the binding point
        bindings_lookup.emplace(resource.binding, layout_binding);

        binding_flags_lookup.emplace(resource.binding, binding_flags.back());

        resources_lookup.emplace(resource.name, resource.binding);
    }

    VkDescriptorSetLayoutCreateInfo create_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    create_info.flags        = 0;
    create_info.bindingCount = to_u32(bindings.size());
    create_info.pBindings    = bindings.data();

    // Handle update-after-bind extensions
    if (std::find_if(resource_set.begin(), resource_set.end(),
                     [](const ShaderResource& shader_resource) {
                         return shader_resource.mode == ShaderResourceMode::UpdateAfterBind;
                     }) != resource_set.end()) {
        // Spec states you can't have ANY dynamic resources if you have one of the bindings set to update-after-bind
        if (std::find_if(resource_set.begin(), resource_set.end(),
                         [](const ShaderResource& shader_resource) {
                             return shader_resource.mode == ShaderResourceMode::Dynamic;
                         }) != resource_set.end()) {
            throw std::runtime_error(
                "Cannot create descriptor set layout, dynamic resources are not allowed if at least one resource is update-after-bind.");
        }

        if (!validate_flags(device.get_gpu(), bindings, binding_flags)) {
            throw std::runtime_error("Invalid binding, couldn't create descriptor set layout.");
        }

        VkDescriptorSetLayoutBindingFlagsCreateInfoEXT binding_flags_create_info{
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT};
        binding_flags_create_info.bindingCount  = to_u32(binding_flags.size());
        binding_flags_create_info.pBindingFlags = binding_flags.data();

        create_info.pNext = &binding_flags_create_info;
        create_info.flags |=
            std::find(binding_flags.begin(), binding_flags.end(), VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT) !=
            binding_flags.end() ? VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT_EXT : 0;
    }

    // Create the Vulkan descriptor set layout handle
    VkResult result = vkCreateDescriptorSetLayout(device.handle(), &create_info, nullptr, &handle);
    if (result != VK_SUCCESS) {
        throw VulkanException{vk::Result(result), "Cannot create DescriptorSetLayout"};
    }
}

vk_descriptor_set_layout::vk_descriptor_set_layout(vk_descriptor_set_layout&& other) :
    device{other.device},
    shader_modules{other.shader_modules},
    handle{other.handle},
    set_index{other.set_index},
    bindings{std::move(other.bindings)},
    binding_flags{std::move(other.binding_flags)},
    bindings_lookup{std::move(other.bindings_lookup)},
    binding_flags_lookup{std::move(other.binding_flags_lookup)},
    resources_lookup{std::move(other.resources_lookup)}
{
    other.handle = VK_NULL_HANDLE;
}

vk_descriptor_set_layout::~vk_descriptor_set_layout()
{
    // Destroy descriptor set layout
    if (handle != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device.handle(), handle, nullptr);
    }
}

VkDescriptorSetLayout vk_descriptor_set_layout::get_handle() const
{
    return handle;
}

const uint32_t vk_descriptor_set_layout::get_index() const
{
    return set_index;
}

const std::vector<VkDescriptorSetLayoutBinding>& vk_descriptor_set_layout::get_bindings() const
{
    return bindings;
}

const std::vector<VkDescriptorBindingFlagsEXT>& vk_descriptor_set_layout::get_binding_flags() const
{
    return binding_flags;
}

std::unique_ptr<VkDescriptorSetLayoutBinding> vk_descriptor_set_layout::get_layout_binding(uint32_t binding_index) const
{
    auto it = bindings_lookup.find(binding_index);

    if (it == bindings_lookup.end()) {
        return nullptr;
    }

    return std::make_unique<VkDescriptorSetLayoutBinding>(it->second);
}

std::unique_ptr<VkDescriptorSetLayoutBinding>
vk_descriptor_set_layout::get_layout_binding(const std::string& name) const
{
    auto it = resources_lookup.find(name);

    if (it == resources_lookup.end()) {
        return nullptr;
    }

    return get_layout_binding(it->second);
}

VkDescriptorBindingFlagsEXT vk_descriptor_set_layout::get_layout_binding_flag(const uint32_t binding_index) const
{
    auto it = binding_flags_lookup.find(binding_index);

    if (it == binding_flags_lookup.end()) {
        return 0;
    }

    return it->second;
}

const std::vector<ShaderModule*>& vk_descriptor_set_layout::get_shader_modules() const
{
    return shader_modules;
}