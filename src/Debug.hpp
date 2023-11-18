/**
 * @File Debug.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_debug_utils
{
public:
    virtual ~vk_debug_utils() = default;

    virtual void set_debug_name(vk::Device device, vk::ObjectType object_type,
                                uint64_t object_handle, const char* name) const = 0;

    virtual void set_debug_tag(vk::Device device, vk::ObjectType object_type,
                               uint64_t object_handle, uint64_t tag_name,
                               const void* tag_data, size_t tag_data_size) const = 0;
};

class debug_utils_ext_debug_utils final : public vk_debug_utils
{
public:
    ~debug_utils_ext_debug_utils() override = default;

    void set_debug_name(vk::Device device, vk::ObjectType object_type,
                        uint64_t object_handle, const char* name) const override;

    void set_debug_tag(vk::Device device, vk::ObjectType object_type,
                       uint64_t object_handle, uint64_t tag_name,
                       const void* tag_data, size_t tag_data_size) const override;
};

class debug_marker_ext_debug_utils final : public vk_debug_utils
{
public:
    ~debug_marker_ext_debug_utils() override = default;

    void set_debug_name(vk::Device device, vk::ObjectType object_type,
                        uint64_t object_handle, const char* name) const override;

    void set_debug_tag(vk::Device device, vk::ObjectType object_type,
                       uint64_t object_handle, uint64_t tag_name,
                       const void* tag_data, size_t tag_data_size) const override;

};

class dummy_debug_utils final : public vk_debug_utils
{
public:
    ~dummy_debug_utils() override = default;

    inline void set_debug_name(vk::Device, vk::ObjectType, uint64_t, const char*) const override {}

    inline void set_debug_tag(vk::Device, vk::ObjectType, uint64_t, uint64_t, const void*, size_t) const override {}
};