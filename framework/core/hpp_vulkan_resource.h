/* Copyright (c) 2022-2023, NVIDIA CORPORATION. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 the "License";
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <utility>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class device;

        template <typename THandle, typename TDevice = vkb::core::device>
        class vk_unit
        {
        public:
            vk_unit(THandle handle = nullptr, TDevice* device = nullptr) :
                handle_{handle}, device_{device}
            {
            }

            vk_unit(const vk_unit&) = delete;
            vk_unit& operator=(const vk_unit&) = delete;

            vk_unit(vk_unit&& other) :
                handle_(std::exchange(other.handle_, {})), device_(std::exchange(other.device_, {}))
            {
                set_debug_name(std::exchange(other.debug_name_, {}));
            }

            vk_unit& operator=(vk_unit&& other)
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

            inline TDevice const& get_device() const
            {
                assert(device_ && "VKBDevice handle not set");
                return *device_;
            }

            inline TDevice& get_device()
            {
                assert(device_ && "VKBDevice handle not set");
                return *device_;
            }

            inline const THandle& get_handle() const
            {
                return handle_;
            }

            inline THandle& get_handle()
            {
                return handle_;
            }

            inline uint64_t get_handle_u64() const
            {
                using UintHandle = std::conditional_t<sizeof(THandle) == sizeof(uint32_t), uint32_t, uint64_t>;

                return static_cast<uint64_t>(*reinterpret_cast<UintHandle const*>(&handle_));
            }

            inline void set_handle(THandle hdl)
            {
                handle_ = hdl;
            }

            inline const std::string& get_debug_name() const
            {
                return debug_name_;
            }

            inline void set_debug_name(const std::string& name)
            {
                debug_name_ = name;

                if (device_ && !debug_name_.empty())
                {
                    device_->get_debug_utils().set_debug_name(device_->get_handle(),
                                                              THandle::objectType,
                                                              get_handle_u64(),
                                                              debug_name_.c_str());
                }
            }

        private:
            THandle handle_;
            TDevice* device_;
            std::string debug_name_;
        };
    } // namespace core
} // namespace vkb
