/**
 * @File Queue.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "Queue.hpp"
#include "Device.hpp"
#include "CommandBuffer.hpp"

vk_queue::vk_queue(vk_device& device, uint32_t family_index, vk::QueueFamilyProperties properties,
                   vk::Bool32 can_present, uint32_t index) :
    device{device},
    family_index{family_index},
    index{index},
    can_present{can_present},
    properties{properties}
{
    handle = device.handle().getQueue(family_index, index);
}

vk_queue::vk_queue(vk_queue&& other) :
    device(other.device),
    handle(std::exchange(other.handle, {})),
    family_index(std::exchange(other.family_index, {})),
    index(std::exchange(other.index, 0)),
    can_present(std::exchange(other.can_present, false)),
    properties(std::exchange(other.properties, {})) {}

const vk_device& vk_queue::get_device() const
{
    return device;
}

vk::Queue vk_queue::get_handle() const
{
    return handle;
}

uint32_t vk_queue::get_family_index() const
{
    return family_index;
}

uint32_t vk_queue::get_index() const
{
    return index;
}

const vk::QueueFamilyProperties& vk_queue::get_properties() const
{
    return properties;
}

vk::Bool32 vk_queue::support_present() const
{
    return can_present;
}

void vk_queue::submit(const vk_command_buffer& command_buffer, vk::Fence fence) const
{
    vk::CommandBuffer commandBuffer = command_buffer.handle();
    vk::SubmitInfo    submit_info({}, {}, commandBuffer);
    handle.submit(submit_info, fence);
}

vk::Result vk_queue::present(const vk::PresentInfoKHR& present_info) const
{
    if (!can_present) {
        return vk::Result::eErrorIncompatibleDisplayKHR;
    }

    return handle.presentKHR(present_info);
}