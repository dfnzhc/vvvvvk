/**
 * @File VkUnit.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"

class vk_device;

template<typename THandle, typename TDevice = vk_device>
class vk_unit
{
public:
    explicit vk_unit(THandle handle = nullptr, TDevice* device = nullptr) :
        handle_{handle}, device_{device}
    {
    }

    vk_unit(const vk_unit&) = delete;
    vk_unit& operator=(const vk_unit&) = delete;

    vk_unit(vk_unit&& other) noexcept :
        handle_(std::exchange(other.handle_, {})), device_(std::exchange(other.device_, {}))
    {
        set_debug_name(std::exchange(other.debug_name_, {}));
    }

    vk_unit& operator=(vk_unit&& other) noexcept
    {
        handle_ = std::exchange(other.handle_, {});
        device_ = std::exchange(other.device_, {});
        set_debug_name(std::exchange(other.debug_name_, {}));

        return *this;
    }

    virtual ~vk_unit() = default;

    inline vk::ObjectType get_object_type() const
    {
        return THandle::NativeType;
    }

    inline TDevice const& device() const
    {
        assert(device_ && "TDevice handle not set");
        return *device_;
    }

    inline TDevice& device()
    {
        assert(device_ && "TDevice handle not set");
        return *device_;
    }

    inline const THandle& handle() const
    {
        return handle_;
    }

    inline THandle& handle()
    {
        return handle_;
    }

    inline uint64_t handle_u64() const
    {
        using UintHandle = typename std::conditional_t<sizeof(THandle) == sizeof(uint32_t), uint32_t, uint64_t>;

        return static_cast<uint64_t>(*reinterpret_cast<UintHandle const*>(&handle_));
    }

    inline void set_handle(THandle hdl)
    {
        handle_ = hdl;
    }

    inline const std::string& debug_name() const
    {
        return debug_name_;
    }

    inline void set_debug_name(const std::string& name)
    {
        debug_name_ = name;

        if (device_ && !debug_name_.empty()) {
            device_->get_debug_utils().set_debug_name(device_->handle(), THandle::objectType, handle_u64(),
                                                      debug_name_.c_str());
        }
    }

private:
    THandle handle_;
    TDevice* device_;
    std::string debug_name_;
};