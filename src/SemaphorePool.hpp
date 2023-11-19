/**
 * @File SemaphorePool.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

class vk_semaphore_pool
{
public:
    vk_semaphore_pool(vk_device& device);

    ~vk_semaphore_pool();

    vk_semaphore_pool(const vk_semaphore_pool&) = delete;
    vk_semaphore_pool(vk_semaphore_pool&& other) = delete;

    vk_semaphore_pool& operator=(const vk_semaphore_pool&) = delete;
    vk_semaphore_pool& operator=(vk_semaphore_pool&&) = delete;

    vk::Semaphore request_semaphore();
    vk::Semaphore request_semaphore_with_ownership();
    void release_owned_semaphore(vk::Semaphore semaphore);

    void reset();

    uint32_t get_active_semaphore_count() const;

private:
    vk_device& device;

    std::vector<vk::Semaphore> semaphores;
    std::vector<vk::Semaphore> released_semaphores;

    uint32_t active_semaphore_count{0};
};