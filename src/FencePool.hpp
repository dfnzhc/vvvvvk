/**
 * @File FencePool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_fence_pool
{
public:
    vk_fence_pool(vk_device& device);

    ~vk_fence_pool();

    vk_fence_pool(const vk_fence_pool&) = delete;
    vk_fence_pool(vk_fence_pool&& other) = delete;

    vk_fence_pool& operator=(const vk_fence_pool&) = delete;
    vk_fence_pool& operator=(vk_fence_pool&&) = delete;

    vk::Fence request_fence();

    vk::Result wait(uint32_t timeout = std::numeric_limits<uint32_t>::max()) const;

    vk::Result reset();

private:
    vk_device& device;

    std::vector<vk::Fence> fences;

    uint32_t active_fence_count{0};
};