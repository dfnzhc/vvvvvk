#pragma once

#include <common/glm_common.h>
#include <vulkan/vulkan.hpp>

namespace vkb
{
    namespace core
    {
        class command_buffer;

        class debug_utils
        {
        public:
            virtual ~debug_utils() = default;

            virtual void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const = 0;

            virtual void set_debug_tag(
                vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const = 0;

            virtual void begin_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color = {}) const = 0;
            virtual void end_label(vk::CommandBuffer command_buffer) const = 0;

            virtual void insert_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color = {}) const = 0;
        };

        class debug_utils_ext_debug_utils final : public vkb::core::debug_utils
        {
        public:
            ~debug_utils_ext_debug_utils() override = default;

            void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const override;
            void set_debug_tag(
                vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const override;

            void begin_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
            void end_label(vk::CommandBuffer command_buffer) const override;

            void insert_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
        };

        class debug_marker_ext_debug_utils final : public vkb::core::debug_utils
        {
        public:
            ~debug_marker_ext_debug_utils() override = default;

            void set_debug_name(vk::Device device, vk::ObjectType object_type, uint64_t object_handle, const char* name) const override;

            void set_debug_tag(
                vk::Device device, vk::ObjectType object_type, uint64_t object_handle, uint64_t tag_name, const void* tag_data, size_t tag_data_size) const override;

            void begin_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
            void end_label(vk::CommandBuffer command_buffer) const override;

            void insert_label(vk::CommandBuffer command_buffer, const char* name, glm::vec4 const color) const override;
        };

        class dummy_debug_utils final : public vkb::core::debug_utils
        {
        public:
            ~dummy_debug_utils() override = default;

            inline void set_debug_name(vk::Device, vk::ObjectType, uint64_t, const char*) const override
            {
            }

            inline void set_debug_tag(vk::Device, vk::ObjectType, uint64_t, uint64_t, const void*, size_t) const override
            {
            }

            inline void begin_label(vk::CommandBuffer, const char*, glm::vec4 const) const override
            {
            }

            inline void end_label(vk::CommandBuffer) const override
            {
            }

            inline void insert_label(vk::CommandBuffer, const char*, glm::vec4 const) const override
            {
            }
        };

        class scoped_debug_label final
        {
        public:
            scoped_debug_label(const vkb::core::debug_utils& debug_utils, vk::CommandBuffer command_buffer, std::string const& name, glm::vec4 const color = {});
            scoped_debug_label(const vkb::core::command_buffer& command_buffer, std::string const& name, glm::vec4 const color = {});

            ~scoped_debug_label();

        private:
            const vkb::core::debug_utils* debug_utils_;
            vk::CommandBuffer command_buffer_;
        };
    } // namespace core
} // namespace vkb
