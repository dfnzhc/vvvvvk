/**
 * @File RenderContext.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include <set>
#include "VkCommon.hpp"
#include "RenderTarget.hpp"
#include "CommandBuffer.hpp"
#include "Queue.hpp"
#include "SwapChain.hpp"
#include "RenderFrame.hpp"

class vk_device;

class vk_render_context
{
public:
    // The format to use for the RenderTargets if a swapchain isn't created
    inline static vk::Format DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

    // @formatter:off
    vk_render_context(vk_device                               &device,
                     vk::SurfaceKHR                           surface,
                     const vk::Extent2D                      &extent,
                     vk::PresentModeKHR                       present_mode                 = vk::PresentModeKHR::eFifo,
                     std::vector<vk::PresentModeKHR> const   &present_mode_priority_list   = {vk::PresentModeKHR::eFifo, vk::PresentModeKHR::eMailbox},
                     std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list = {
                         {vk::Format::eR8G8B8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}, {vk::Format::eB8G8R8A8Srgb, vk::ColorSpaceKHR::eSrgbNonlinear}});
    // @formatter:on

    virtual ~vk_render_context() = default;

    vk_render_context(const vk_render_context&) = delete;
    vk_render_context(vk_render_context&&) = delete;

    vk_render_context& operator=(const vk_render_context&) = delete;
    vk_render_context& operator=(vk_render_context&&) = delete;

    void prepare(size_t thread_count = 1,
                 vk_render_target::CreateFunc create_render_target_func = vk_render_target::DEFAULT_CREATE_FUNC);

    void update_swapchain(const vk::Extent2D& extent);

    void update_swapchain(const uint32_t image_count);

    void update_swapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags);

    void update_swapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform);

    bool has_swapchain();

    void recreate();

    void recreate_swapchain();

    vk_command_buffer& begin(vk_command_buffer::reset_mode reset_mode = vk_command_buffer::reset_mode::ResetPool);

    void submit(vk_command_buffer& command_buffer);

    void submit(const std::vector<vk_command_buffer*>& command_buffers);

    void begin_frame();

    vk::Semaphore submit(const vk_queue& queue,
                         const std::vector<vk_command_buffer*>& command_buffers,
                         vk::Semaphore wait_semaphore,
                         vk::PipelineStageFlags wait_pipeline_stage);

    void submit(const vk_queue& queue, const std::vector<vk_command_buffer*>& command_buffers);

    virtual void wait_frame();

    void end_frame(vk::Semaphore semaphore);

    vk_render_frame& get_active_frame();

    uint32_t get_active_frame_index();

    vk_render_frame& get_last_rendered_frame();

    vk::Semaphore request_semaphore();
    vk::Semaphore request_semaphore_with_ownership();
    void release_owned_semaphore(vk::Semaphore semaphore);

    vk_device& get_device();

    vk::Format get_format() const;

    const vk_swapchain& get_swapchain() const;

    const vk::Extent2D& get_surface_extent() const;

    uint32_t get_active_frame_index() const;

    std::vector<std::unique_ptr<vk_render_frame>>& get_render_frames();

    virtual bool handle_surface_changes(bool force_update = false);

    vk::Semaphore consume_acquired_semaphore();

protected:
    vk::Extent2D surface_extent;

private:
    vk_device& device;

    const vk_queue& queue;

    std::unique_ptr<vk_swapchain> swapchain;

    swapchain_desc swapchain_properties;

    std::vector<std::unique_ptr<vk_render_frame>> frames;

    vk::Semaphore acquired_semaphore;

    bool prepared{false};

    uint32_t active_frame_index{0};

    bool frame_active{false};

    vk_render_target::CreateFunc create_render_target_func = vk_render_target::DEFAULT_CREATE_FUNC;

    vk::SurfaceTransformFlagBitsKHR pre_transform{vk::SurfaceTransformFlagBitsKHR::eIdentity};

    size_t thread_count{1};
};