/**
 * @File ShaderModule.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "Helpers.hpp"

class vk_device;

enum class ShaderResourceType
{
    Input,
    InputAttachment,
    Output,
    Image,
    ImageSampler,
    ImageStorage,
    Sampler,
    BufferUniform,
    BufferStorage,
    PushConstant,
    SpecializationConstant,
    All
};

enum class ShaderResourceMode
{
    Static,
    Dynamic,
    UpdateAfterBind
};

struct ShaderResourceQualifiers
{
    enum : uint32_t
    {
        None        = 0,
        NonReadable = 1,
        NonWritable = 2,
    };
};

struct ShaderResource
{
    VkShaderStageFlags stages;

    ShaderResourceType type;

    ShaderResourceMode mode;

    uint32_t set;

    uint32_t binding;

    uint32_t location;

    uint32_t input_attachment_index;

    uint32_t vec_size;

    uint32_t columns;

    uint32_t array_size;

    uint32_t offset;

    uint32_t size;

    uint32_t constant_id;

    uint32_t qualifiers;

    std::string name;
};

class ShaderVariant
{
public:
    ShaderVariant() = default;

    ShaderVariant(std::string&& preamble, std::vector<std::string>&& processes);

    size_t get_id() const;

    void add_definitions(const std::vector<std::string>& definitions);

    void add_define(const std::string& def);

    void add_undefine(const std::string& undef);

    void add_runtime_array_size(const std::string& runtime_array_name, size_t size);

    void set_runtime_array_sizes(const std::unordered_map<std::string, size_t>& sizes);

    const std::string& get_preamble() const;

    const std::vector<std::string>& get_processes() const;

    const std::unordered_map<std::string, size_t>& get_runtime_array_sizes() const;

    void clear();

private:
    size_t id;

    std::string preamble;

    std::vector<std::string> processes;

    std::unordered_map<std::string, size_t> runtime_array_sizes;

    void update_id();
};

class ShaderSource
{
public:
    ShaderSource() = default;

    ShaderSource(const std::string& filename);

    size_t get_id() const;

    const std::string& get_filename() const;

    void set_source(const std::string& source);

    const std::string& get_source() const;

private:
    size_t id;

    std::string filename;

    std::string source;
};

class ShaderModule
{
public:
    ShaderModule(vk_device& device,
                 VkShaderStageFlagBits stage,
                 const ShaderSource& glsl_source,
                 const std::string& entry_point,
                 const ShaderVariant& shader_variant);

    ShaderModule(const ShaderModule&) = delete;

    ShaderModule(ShaderModule&& other) noexcept;

    ShaderModule& operator=(const ShaderModule&) = delete;

    ShaderModule& operator=(ShaderModule&&) = delete;

    size_t get_id() const;

    VkShaderStageFlagBits get_stage() const;

    const std::string& get_entry_point() const;

    const std::vector<ShaderResource>& get_resources() const;

    const std::string& get_info_log() const;

    const std::vector<uint32_t>& get_binary() const;

    inline const std::string& get_debug_name() const
    {
        return debug_name;
    }

    inline void set_debug_name(const std::string& name)
    {
        debug_name = name;
    }

    void set_resource_mode(const std::string& resource_name, const ShaderResourceMode& resource_mode);

private:
    vk_device& device;

    /// Shader unique id
    size_t id;

    /// Stage of the shader (vertex, fragment, etc)
    VkShaderStageFlagBits stage{};

    /// Name of the main function
    std::string entry_point;

    /// Human-readable name for the shader
    std::string debug_name;

    /// Compiled source
    std::vector<uint32_t> spirv;

    std::vector<ShaderResource> resources;

    std::string info_log;
};