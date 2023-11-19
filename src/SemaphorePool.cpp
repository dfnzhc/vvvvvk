/**
 * @File SemaphorePool.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "SemaphorePool.hpp"
#include "Device.hpp"

vk_semaphore_pool::vk_semaphore_pool(vk_device& device) :
    device{device}
{
}

vk_semaphore_pool::~vk_semaphore_pool()
{
    reset();

    for (vk::Semaphore semaphore: semaphores) {
        device.handle().destroySemaphore(semaphore, nullptr);
    }

    semaphores.clear();
}

vk::Semaphore vk_semaphore_pool::request_semaphore_with_ownership()
{
    // 如果有可用的，直接返回
    if (active_semaphore_count < semaphores.size()) {
        vk::Semaphore semaphore = semaphores.back();
        semaphores.pop_back();
        return semaphore;
    }
    
    // 创建新的信号量，由调用者负责管理
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo create_info;
    VK_CHECK(device.handle().createSemaphore(&create_info, nullptr, &semaphore));

    return semaphore;
}

void vk_semaphore_pool::release_owned_semaphore(vk::Semaphore semaphore)
{
    // 在调用 reset 之前不可用
    released_semaphores.push_back(semaphore);
}

vk::Semaphore vk_semaphore_pool::request_semaphore()
{
    // 如果有可用的，直接返回
    if (active_semaphore_count < semaphores.size()) {
        return semaphores[active_semaphore_count++];
    }

    // 创建新的信号量，并追踪该信号量
    vk::Semaphore semaphore;
    vk::SemaphoreCreateInfo create_info;
    VK_CHECK(device.handle().createSemaphore(&create_info, nullptr, &semaphore));

    semaphores.push_back(semaphore);
    active_semaphore_count++;

    return semaphore;
}

void vk_semaphore_pool::reset()
{
    active_semaphore_count = 0;

    // 可以继续重用
    for (auto& sem: released_semaphores) {
        semaphores.push_back(sem);
    }

    released_semaphores.clear();
}

uint32_t vk_semaphore_pool::get_active_semaphore_count() const
{
    return active_semaphore_count;
}