/**
 * @File Queue.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_command_buffer;

class vk_queue
{
public:
    vk_queue(vk_device& device, uint32_t family_index, vk::QueueFamilyProperties properties, vk::Bool32 can_present,
             uint32_t index);

    vk_queue(const vk_queue&) = default;

    vk_queue(vk_queue&& other);

    vk_queue& operator=(const vk_queue&) = delete;

    vk_queue& operator=(vk_queue&&) = delete;

    const vk_device& get_device() const;

    vk::Queue get_handle() const;

    uint32_t get_family_index() const;

    uint32_t get_index() const;

    const vk::QueueFamilyProperties& get_properties() const;

    vk::Bool32 support_present() const;

    void submit(const vk_command_buffer& command_buffer, vk::Fence fence) const;

    vk::Result present(const vk::PresentInfoKHR& present_infos) const;

private:
    vk_device& device;

    vk::Queue handle;

    uint32_t family_index{0};

    uint32_t index{0};

    vk::Bool32 can_present = false;

    vk::QueueFamilyProperties properties{};
};