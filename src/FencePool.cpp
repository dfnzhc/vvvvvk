/**
 * @File FencePool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "FencePool.hpp"
#include "Device.hpp"

vk_fence_pool::vk_fence_pool(vk_device& device) :
    device{device}
{
}

vk_fence_pool::~vk_fence_pool()
{
    wait();
    reset();

    for (vk::Fence fence: fences) {
        device.handle().destroyFence(fence);
    }

    fences.clear();
}

vk::Fence vk_fence_pool::request_fence()
{
    // 直接返回可用 fence
    if (active_fence_count < fences.size()) {
        return fences[active_fence_count++];
    }

    // 创建新的 fence
    vk::Fence fence{VK_NULL_HANDLE};

    vk::FenceCreateInfo create_info;

    VK_CHECK(device.handle().createFence(&create_info, nullptr, &fence));

    fences.push_back(fence);
    active_fence_count++;

    return fences.back();
}

vk::Result vk_fence_pool::wait(uint32_t timeout) const
{
    if (active_fence_count < 1 || fences.empty()) {
        return vk::Result::eSuccess;
    }

    return device.handle().waitForFences(active_fence_count, fences.data(), true, timeout);
}

vk::Result vk_fence_pool::reset()
{
    if (active_fence_count < 1 || fences.empty()) {
        return vk::Result::eSuccess;
    }

    vk::Result result = device.handle().resetFences(active_fence_count, fences.data());
    if (result != vk::Result::eSuccess) {
        return result;
    }

    active_fence_count = 0;
    return vk::Result::eSuccess;
}