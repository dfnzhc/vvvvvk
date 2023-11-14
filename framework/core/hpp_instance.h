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

#include "common/optional.h"
#include <common/hpp_error.h>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class physical_device;

        std::vector<const char*>
        get_optimal_validation_layers(const std::vector<vk::LayerProperties>& supported_instance_layers);

        class instance
        {
        public:
            static Optional<uint32_t> selected_gpu_index;

            instance(VkInstance instance,
                     const std::vector<const char*>& extensions = {},
                     const std::vector<const char*>& layers = {});

            instance(vk::Instance instance);

            instance(const instance&) = delete;

            instance(instance&&) = delete;

            ~instance();

            instance& operator=(const instance&) = delete;

            instance& operator=(instance&&) = delete;

            const std::vector<const char*>& get_extensions();

            physical_device& get_first_gpu();

            vk::Instance get_handle() const;

            physical_device& get_suitable_gpu(vk::SurfaceKHR);
            bool is_enabled(const char* extension) const;

        private:
            void query_gpus();

        private:
            vk::Instance handle_;

            std::vector<const char*> extensions_;
            std::vector<const char*> layers_;

#if defined(VKB_DEBUG) || defined(VKB_VALIDATION_LAYERS)
            vk::DebugUtilsMessengerEXT debug_utils_messenger_;
            vk::DebugReportCallbackEXT debug_report_callback_;
#endif
            std::vector<std::unique_ptr<physical_device>> gpus_;
        };
    } // namespace core
} // namespace vkb
