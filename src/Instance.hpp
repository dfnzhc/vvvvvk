/**
 * @File VkInstance.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include <unordered_set>
#include "VkCommon.hpp"

class vk_instance
{
public:

    vk_instance(vk::Instance instance,
                const std::vector<const char*>& enabled_extensions = {});

    ~vk_instance();

    vk_instance(const vk_instance&) = delete;
    vk_instance(vk_instance&&) = delete;
    vk_instance& operator=(const vk_instance&) = delete;
    vk_instance& operator=(vk_instance&&) = delete;

    const std::vector<const char*>& extensions();

    vk::Instance handle() const;

    bool is_enabled(const char* extension) const;

private:
    void query_gpus();
    void init_debug_utils();

private:
    vk::Instance handle_;

    std::vector<const char*> enabled_extensions_;

    vk::DebugUtilsMessengerEXT          debug_messenger_;
};