#pragma once

#include <core/hpp_device.h>
#include <core/hpp_swapchain.h>
#include <platform/window.h>
#include <rendering/hpp_render_frame.h>

namespace vkb
{
    namespace rendering
    {
        class render_context
        {
        public:
            static vk::Format DEFAULT_VK_FORMAT;

            render_context(vkb::core::device& device,
                           vk::SurfaceKHR surface,
                           const vkb::Window& window,
                           vk::PresentModeKHR present_mode = vk::PresentModeKHR::eFifo,
                           std::vector<vk::PresentModeKHR> const& present_mode_priority_list = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
                           std::vector<vk::SurfaceFormatKHR> const& surface_format_priority_list = {
                               {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}, {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}
                           });

            virtual ~render_context() = default;

            render_context(const render_context&) = delete;
            render_context(render_context&&) = delete;

            render_context& operator=(const render_context&) = delete;
            render_context& operator=(render_context&&) = delete;

            void prepare(size_t thread_count = 1, render_target::CreateFunc create_render_target_func = render_target::DEFAULT_CREATE_FUNC);

            void update_swapchain(const vk::Extent2D& extent);
            void update_swapchain(const uint32_t image_count);
            void update_swapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags);
            void update_swapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

            bool has_swapchain();

            void recreate();
            void recreate_swapchain();

            vkb::core::command_buffer& begin(vkb::core::command_buffer::reset_mode reset_mode = vkb::core::command_buffer::reset_mode::ResetPool);

            void submit(vkb::core::command_buffer& command_buffer);
            void submit(const std::vector<vkb::core::command_buffer*>& command_buffers);

            vk::Semaphore submit(const vkb::core::queue& queue,
                                 const std::vector<vkb::core::command_buffer*>& command_buffers,
                                 vk::Semaphore wait_semaphore,
                                 vk::PipelineStageFlags wait_pipeline_stage);

            void submit(const vkb::core::queue& queue, const std::vector<vkb::core::command_buffer*>& command_buffers);

            void begin_frame();
            void end_frame(vk::Semaphore semaphore);
            virtual void wait_frame();

            render_frame& active_frame();
            uint32_t active_frame_index();
            render_frame& last_rendered_frame();

            vk::Semaphore request_semaphore();
            vk::Semaphore request_semaphore_with_ownership();
            void release_owned_semaphore(vk::Semaphore semaphore);

            vkb::core::device& get_device();
            vk::Format format() const;

            vkb::core::swapchain const& swapchain() const;
            vk::Extent2D const& surface_extent() const;

            uint32_t active_frame_index() const;
            std::vector<std::unique_ptr<render_frame>>& render_frames();

            virtual bool handle_surface_changes(bool force_update = false);

            vk::Semaphore consume_acquired_semaphore();

        protected:
            vk::Extent2D surface_extent_;

        private:
            vkb::core::device& device_;
            const vkb::Window& window_;

            const vkb::core::queue& queue_;

            std::unique_ptr<vkb::core::swapchain> swapchain_;
            vkb::core::HPPSwapchainProperties swapchain_properties_;
            std::vector<std::unique_ptr<render_frame>> frames_;

            vk::Semaphore acquired_semaphore_;

            bool prepared_{false};

            uint32_t active_frame_index_{0};
            bool frame_active_{false};

            render_target::CreateFunc create_render_target_func_ = render_target::DEFAULT_CREATE_FUNC;
            vk::SurfaceTransformFlagBitsKHR pre_transform_{vk::SurfaceTransformFlagBitsKHR::eIdentity};

            size_t thread_count_{1};
        };
    } // namespace rendering
} // namespace vkb
