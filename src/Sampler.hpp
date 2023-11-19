/**
 * @File Sampler.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"

class vk_sampler : public vk_unit<vk::Sampler>
{
public:
    vk_sampler(vk_device &device, const vk::SamplerCreateInfo &info);
    vk_sampler(vk_sampler &&sampler);

    ~vk_sampler();

    vk_sampler(const vk_sampler &) = delete;
    
    vk_sampler &operator=(const vk_sampler &) = delete;
    vk_sampler &operator=(vk_sampler &&) = delete;
};