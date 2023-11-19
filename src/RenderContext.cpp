/**
 * @File RenderContext.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "RenderContext.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"
#include "CommandBufferPool.hpp"
#include "ImageView.hpp"

vk_render_context::vk_render_context(vk_device& device, vk::SurfaceKHR surface, const vk::Extent2D& extent,
                                     vk::PresentModeKHR present_mode,
                                     const std::vector<vk::PresentModeKHR>& present_mode_priority_list,
                                     const std::vector<vk::SurfaceFormatKHR>& surface_format_priority_list)
    : device{device}, queue{device.get_suitable_graphics_queue()}, surface_extent{extent}
{
    if (surface) {
        vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().handle().getSurfaceCapabilitiesKHR(surface);

        if (surface_properties.currentExtent.width == 0xFFFFFFFF) {
            swapchain =
                std::make_unique<vk_swapchain>(device, surface, present_mode, present_mode_priority_list,
                                               surface_format_priority_list, surface_extent);
        } else {
            swapchain = std::make_unique<vk_swapchain>(device, surface, present_mode, present_mode_priority_list,
                                                       surface_format_priority_list);
        }
    }
}

void vk_render_context::prepare(size_t thread_count, vk_render_target::CreateFunc create_render_target_func)
{
    device.handle().waitIdle();

    if (swapchain) {
        surface_extent = swapchain->extent();

        vk::Extent3D extent{surface_extent.width, surface_extent.height, 1};

        for (auto& image_handle: swapchain->images()) {
            auto swapchain_image = vk_image{device, image_handle, extent, swapchain->format(), swapchain->usage()};
            auto render_target   = create_render_target_func(std::move(swapchain_image));
            frames.emplace_back(std::make_unique<vk_render_frame>(device, std::move(render_target), thread_count));
        }
    } else {
        // Otherwise, create a single RenderFrame
        swapchain = nullptr;

        auto color_image = vk_image{device,
                                    vk::Extent3D{surface_extent.width, surface_extent.height, 1},
                                    DEFAULT_VK_FORMAT,        // We can use any format here that we like
                                    vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
                                    VMA_MEMORY_USAGE_GPU_ONLY};

        auto render_target = create_render_target_func(std::move(color_image));
        frames.emplace_back(std::make_unique<vk_render_frame>(device, std::move(render_target), thread_count));
    }

    this->create_render_target_func = create_render_target_func;
    this->thread_count              = thread_count;
    this->prepared                  = true;
}

void vk_render_context::update_swapchain(const vk::Extent2D& extent)
{
    if (!swapchain) {
        LOGW("Can't update the swapchains extent in headless mode, skipping.");
        return;
    }

//    device.get_resource_cache().clear_framebuffers();

    swapchain = std::make_unique<vk_swapchain>(*swapchain, extent);
    recreate();
}

void vk_render_context::update_swapchain(const uint32_t image_count)
{
    if (!swapchain) {
        LOGW("Can't update the swapchains image count in headless mode, skipping.");
        return;
    }

//    device.get_resource_cache().clear_framebuffers();

    device.handle().waitIdle();
    swapchain = std::make_unique<vk_swapchain>(*swapchain, image_count);

    recreate();

}

void vk_render_context::update_swapchain(const std::set<vk::ImageUsageFlagBits>& image_usage_flags)
{
    if (!swapchain) {
        LOGW("Can't update the swapchains image usage in headless mode, skipping.");
        return;
    }

//    device.get_resource_cache().clear_framebuffers();

    swapchain = std::make_unique<vk_swapchain>(*swapchain, image_usage_flags);
    recreate();
}

void vk_render_context::update_swapchain(const vk::Extent2D& extent, const vk::SurfaceTransformFlagBitsKHR transform)
{
    if (!swapchain) {
        LOGW("Can't update the swapchains extent and surface transform in headless mode, skipping.");
        return;
    }

//    device.get_resource_cache().clear_framebuffers();

    auto width  = extent.width;
    auto height = extent.height;
    if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90
        || transform == vk::SurfaceTransformFlagBitsKHR::eRotate270) {
        // Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
        std::swap(width, height);
    }

    swapchain = std::make_unique<vk_swapchain>(*swapchain, vk::Extent2D{width, height}, transform);

    // Save the preTransform attribute for future rotations
    pre_transform = transform;

    recreate();
}

bool vk_render_context::has_swapchain()
{
    return swapchain != nullptr;
}

void vk_render_context::recreate()
{
    LOGI("Recreated swapchain");

    vk::Extent2D swapchain_extent = swapchain->extent();
    vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

    auto frame_it = frames.begin();

    for (auto& image_handle: swapchain->images()) {

        vk_image swapchain_image{device, image_handle, extent, swapchain->format(), swapchain->usage()};
        auto     render_target = create_render_target_func(std::move(swapchain_image));

        if (frame_it != frames.end()) {
            (*frame_it)->update_render_target(std::move(render_target));
        } else {
            // Create a new frame if the new swapchain has more images than current frames
            frames.emplace_back(std::make_unique<vk_render_frame>(device, std::move(render_target), thread_count));
        }

        ++frame_it;
    }

//    device.get_resource_cache().clear_framebuffers();
}

void vk_render_context::recreate_swapchain()
{
    device.handle().waitIdle();
//    device.get_resource_cache().clear_framebuffers();

    vk::Extent2D swapchain_extent = swapchain->extent();
    vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

    auto frame_it = frames.begin();

    for (auto& image_handle: swapchain->images()) {
        vk_image swapchain_image{device, image_handle, extent, swapchain->format(), swapchain->usage()};
        auto     render_target = create_render_target_func(std::move(swapchain_image));
        (*frame_it)->update_render_target(std::move(render_target));

        ++frame_it;
    }
}

vk_command_buffer& vk_render_context::begin(vk_command_buffer::reset_mode reset_mode)
{
    assert(prepared && "HPPRenderContext not prepared for rendering, call prepare()");

    if (!frame_active) {
        begin_frame();
    }

    if (!acquired_semaphore) {
        throw std::runtime_error("Couldn't begin frame");
    }

    const auto& queue = device.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
    return get_active_frame().request_command_buffer(queue, reset_mode);
}

void vk_render_context::submit(vk_command_buffer& command_buffer)
{
    submit({&command_buffer});
}

void vk_render_context::submit(const std::vector<vk_command_buffer*>& command_buffers)
{
    assert(frame_active && "HPPRenderContext is inactive, cannot submit command buffer. Please call begin()");

    vk::Semaphore render_semaphore;

    if (swapchain) {
        assert(acquired_semaphore && "We do not have acquired_semaphore, it was probably consumed?\n");
        render_semaphore = submit(queue, command_buffers, acquired_semaphore,
                                  vk::PipelineStageFlagBits::eColorAttachmentOutput);
    } else {
        submit(queue, command_buffers);
    }

    end_frame(render_semaphore);
}

void vk_render_context::begin_frame()
{
    // Only handle surface changes if a swapchain exists
    if (swapchain) {
        handle_surface_changes();
    }

    assert(!frame_active && "Frame is still active, please call end_frame");

    auto& prev_frame = *frames[active_frame_index];

    // We will use the acquired semaphore in a different frame context,
    // so we need to hold ownership.
    acquired_semaphore = prev_frame.request_semaphore_with_ownership();

    if (swapchain) {
        vk::Result result;
        try {
            std::tie(result, active_frame_index) = swapchain->acquire(acquired_semaphore);
        }
        catch (vk::OutOfDateKHRError& /*err*/) {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
            bool swapchain_updated = handle_surface_changes(result == vk::Result::eErrorOutOfDateKHR);

            if (swapchain_updated) {
                std::tie(result, active_frame_index) = swapchain->acquire(acquired_semaphore);
            }
        }

        if (result != vk::Result::eSuccess) {
            prev_frame.reset();
            return;
        }
    }

    // Now the frame is active again
    frame_active = true;

    // Wait on all resource to be freed from the previous render to this frame
    wait_frame();
}

vk::Semaphore vk_render_context::submit(const vk_queue& queue, const std::vector<vk_command_buffer*>& command_buffers,
                                        vk::Semaphore wait_semaphore, vk::PipelineStageFlags wait_pipeline_stage)
{
    std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
    std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(),
                   [](const vk_command_buffer* cmd_buf) { return cmd_buf->handle(); });

    auto& frame = get_active_frame();

    vk::Semaphore signal_semaphore = frame.request_semaphore();

    vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles, signal_semaphore);
    if (wait_semaphore) {
        submit_info.setWaitSemaphores(wait_semaphore);
        submit_info.pWaitDstStageMask = &wait_pipeline_stage;
    }

    vk::Fence fence = frame.request_fence();

    queue.get_handle().submit(submit_info, fence);

    return signal_semaphore;
}

void vk_render_context::submit(const vk_queue& queue, const std::vector<vk_command_buffer*>& command_buffers)
{
    std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
    std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(),
                   [](const vk_command_buffer* cmd_buf) { return cmd_buf->handle(); });

    auto& frame = get_active_frame();

    vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles);

    vk::Fence fence = frame.request_fence();

    queue.get_handle().submit(submit_info, fence);
}

void vk_render_context::wait_frame()
{
    get_active_frame().reset();
}

void vk_render_context::end_frame(vk::Semaphore semaphore)
{
    assert(frame_active && "Frame is not active, please call begin_frame");

    if (swapchain) {
        vk::SwapchainKHR   vk_swapchain = swapchain->handle();
        vk::PresentInfoKHR present_info(semaphore, vk_swapchain, active_frame_index);

        vk::Result result;
        try {
            result = queue.present(present_info);
        }
        catch (vk::OutOfDateKHRError& /*err*/) {
            result = vk::Result::eErrorOutOfDateKHR;
        }

        if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR) {
            handle_surface_changes();
        }
    }

    // Frame is not active anymore
    if (acquired_semaphore) {
        release_owned_semaphore(acquired_semaphore);
        acquired_semaphore = nullptr;
    }
    frame_active = false;
}

vk_render_frame& vk_render_context::get_active_frame()
{
    assert(frame_active && "Frame is not active, please call begin_frame");
    return *frames[active_frame_index];
}

uint32_t vk_render_context::get_active_frame_index()
{
    assert(frame_active && "Frame is not active, please call begin_frame");
    return active_frame_index;
}

vk_render_frame& vk_render_context::get_last_rendered_frame()
{
    assert(!frame_active && "Frame is still active, please call end_frame");
    return *frames[active_frame_index];
}

vk::Semaphore vk_render_context::request_semaphore()
{
    return get_active_frame().request_semaphore();
}

vk::Semaphore vk_render_context::request_semaphore_with_ownership()
{
    return get_active_frame().request_semaphore_with_ownership();
}

void vk_render_context::release_owned_semaphore(vk::Semaphore semaphore)
{
    get_active_frame().release_owned_semaphore(semaphore);
}

vk_device& vk_render_context::get_device()
{
    return device;
}

vk::Format vk_render_context::get_format() const
{
    return swapchain ? swapchain->format() : DEFAULT_VK_FORMAT;
}

const vk_swapchain& vk_render_context::get_swapchain() const
{
    assert(swapchain && "Swapchain is not valid");
    return *swapchain;
}

const vk::Extent2D& vk_render_context::get_surface_extent() const
{
    return surface_extent;
}

uint32_t vk_render_context::get_active_frame_index() const
{
    assert(frame_active && "Frame is not active, please call begin_frame");
    return active_frame_index;
}

std::vector<std::unique_ptr<vk_render_frame>>& vk_render_context::get_render_frames()
{
    return frames;
}

bool vk_render_context::handle_surface_changes(bool force_update)
{
    if (!swapchain) {
        LOGW("Can't handle surface changes in headless mode, skipping.");
        return false;
    }

    vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().handle().getSurfaceCapabilitiesKHR(
        swapchain->surface());

    if (surface_properties.currentExtent.width == 0xFFFFFFFF) {
        return false;
    }

    if (surface_properties.currentExtent.width != surface_extent.width ||
        surface_properties.currentExtent.height != surface_extent.height ||
        force_update) {
        // Recreate swapchain
        device.handle().waitIdle();

        update_swapchain(surface_properties.currentExtent, pre_transform);

        surface_extent = surface_properties.currentExtent;

        return true;
    }

    return false;
}

vk::Semaphore vk_render_context::consume_acquired_semaphore()
{
    assert(frame_active && "Frame is not active, please call begin_frame");
    return std::exchange(acquired_semaphore, nullptr);
}
