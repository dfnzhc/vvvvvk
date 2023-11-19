/**
 * @File Sampler.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "Sampler.hpp"
#include "Debug.hpp"
#include "Device.hpp"

vk_sampler::vk_sampler(vk_device& device, const vk::SamplerCreateInfo& info) :
    vk_unit{device.handle().createSampler(info), &device} {}

vk_sampler::vk_sampler(vk_sampler&& other) :
    vk_unit(std::move(other)) {}

vk_sampler::~vk_sampler()
{
    if (handle()) {
        device().handle().destroySampler(handle());
    }
}