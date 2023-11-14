/* Copyright (c) 2023, NVIDIA CORPORATION. All rights reserved.
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

#include "hpp_render_context.h"

#include <core/hpp_image.h>

namespace vkb
{
namespace rendering
{
vk::Format render_context::DEFAULT_VK_FORMAT = vk::Format::eR8G8B8A8Srgb;

render_context::render_context(vkb::core::device                    &device,
                                   vk::SurfaceKHR                           surface,
                                   const vkb::Window                       &window,
                                   vk::PresentModeKHR                       present_mode,
                                   std::vector<vk::PresentModeKHR> const   &present_mode_priority_list,
                                   std::vector<vk::SurfaceFormatKHR> const &surface_format_priority_list) :
    device_{device}, window_{window}, queue_{device.get_suitable_graphics_queue()}, surface_extent_{window.get_extent().width, window.get_extent().height}
{
	if (surface)
	{
		vk::SurfaceCapabilitiesKHR surface_properties = device.get_gpu().get_handle().getSurfaceCapabilitiesKHR(surface);

		if (surface_properties.currentExtent.width == 0xFFFFFFFF)
		{
			swapchain_ =
			    std::make_unique<vkb::core::swapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list, surface_extent_);
		}
		else
		{
			swapchain_ = std::make_unique<vkb::core::swapchain>(device, surface, present_mode, present_mode_priority_list, surface_format_priority_list);
		}
	}
}

void render_context::prepare(size_t thread_count, vkb::rendering::render_target::CreateFunc create_render_target_func)
{
	device_.get_handle().waitIdle();

	if (swapchain_)
	{
		surface_extent_ = swapchain_->extent();

		vk::Extent3D extent{surface_extent_.width, surface_extent_.height, 1};

		for (auto &image_handle : swapchain_->images())
		{
			auto swapchain_image = core::image{device_, image_handle, extent, swapchain_->format(), swapchain_->usage()};
			auto render_target   = create_render_target_func(std::move(swapchain_image));
			frames_.emplace_back(std::make_unique<vkb::rendering::render_frame>(device_, std::move(render_target), thread_count));
		}
	}
	else
	{
		// Otherwise, create a single RenderFrame
		swapchain_ = nullptr;

		auto color_image = vkb::core::image{device_,
		                                       vk::Extent3D{surface_extent_.width, surface_extent_.height, 1},
		                                       DEFAULT_VK_FORMAT,        // We can use any format here that we like
		                                       vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc,
		                                       VMA_MEMORY_USAGE_GPU_ONLY};

		auto render_target = create_render_target_func(std::move(color_image));
		frames_.emplace_back(std::make_unique<vkb::rendering::render_frame>(device_, std::move(render_target), thread_count));
	}

	this->create_render_target_func_ = create_render_target_func;
	this->thread_count_              = thread_count;
	this->prepared_                  = true;
}

vk::Format render_context::format() const
{
	return swapchain_ ? swapchain_->format() : DEFAULT_VK_FORMAT;
}

void render_context::update_swapchain(const vk::Extent2D &extent)
{
	if (!swapchain_)
	{
		LOGW("Can't update the swapchains extent in headless mode, skipping.");
		return;
	}

	device_.get_resource_cache().clear_framebuffers();

	swapchain_ = std::make_unique<vkb::core::swapchain>(*swapchain_, extent);

	recreate();
}

void render_context::update_swapchain(const uint32_t image_count)
{
	if (!swapchain_)
	{
		LOGW("Can't update the swapchains image count in headless mode, skipping.");
		return;
	}

	device_.get_resource_cache().clear_framebuffers();

	device_.get_handle().waitIdle();

	swapchain_ = std::make_unique<vkb::core::swapchain>(*swapchain_, image_count);

	recreate();
}

void render_context::update_swapchain(const std::set<vk::ImageUsageFlagBits> &image_usage_flags)
{
	if (!swapchain_)
	{
		LOGW("Can't update the swapchains image usage in headless mode, skipping.");
		return;
	}

	device_.get_resource_cache().clear_framebuffers();

	swapchain_ = std::make_unique<vkb::core::swapchain>(*swapchain_, image_usage_flags);

	recreate();
}

void render_context::update_swapchain(const vk::Extent2D &extent, const vk::SurfaceTransformFlagBitsKHR transform)
{
	if (!swapchain_)
	{
		LOGW("Can't update the swapchains extent and surface transform in headless mode, skipping.");
		return;
	}

	device_.get_resource_cache().clear_framebuffers();

	auto width  = extent.width;
	auto height = extent.height;
	if (transform == vk::SurfaceTransformFlagBitsKHR::eRotate90 || transform == vk::SurfaceTransformFlagBitsKHR::eRotate270)
	{
		// Pre-rotation: always use native orientation i.e. if rotated, use width and height of identity transform
		std::swap(width, height);
	}

	swapchain_ = std::make_unique<vkb::core::swapchain>(*swapchain_, vk::Extent2D{width, height}, transform);

	// Save the preTransform attribute for future rotations
	pre_transform_ = transform;

	recreate();
}

void render_context::recreate()
{
	LOGI("Recreated swapchain");

	vk::Extent2D swapchain_extent = swapchain_->extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames_.begin();

	for (auto &image_handle : swapchain_->images())
	{
		vkb::core::image swapchain_image{device_, image_handle, extent, swapchain_->format(), swapchain_->usage()};

		auto render_target = create_render_target_func_(std::move(swapchain_image));

		if (frame_it != frames_.end())
		{
			(*frame_it)->update_render_target(std::move(render_target));
		}
		else
		{
			// Create a new frame if the new swapchain has more images than current frames
			frames_.emplace_back(std::make_unique<vkb::rendering::render_frame>(device_, std::move(render_target), thread_count_));
		}

		++frame_it;
	}

	device_.get_resource_cache().clear_framebuffers();
}

bool render_context::handle_surface_changes(bool force_update)
{
	if (!swapchain_)
	{
		LOGW("Can't handle surface changes in headless mode, skipping.");
		return false;
	}

	vk::SurfaceCapabilitiesKHR surface_properties = device_.get_gpu().get_handle().getSurfaceCapabilitiesKHR(swapchain_->surface());

	if (surface_properties.currentExtent.width == 0xFFFFFFFF)
	{
		return false;
	}

	// Only recreate the swapchain if the dimensions have changed;
	// handle_surface_changes() is called on VK_SUBOPTIMAL_KHR,
	// which might not be due to a surface resize
	if (surface_properties.currentExtent.width != surface_extent_.width ||
	    surface_properties.currentExtent.height != surface_extent_.height ||
	    force_update)
	{
		// Recreate swapchain
		device_.get_handle().waitIdle();

		update_swapchain(surface_properties.currentExtent, pre_transform_);

		surface_extent_ = surface_properties.currentExtent;

		return true;
	}

	return false;
}

vkb::core::command_buffer &render_context::begin(vkb::core::command_buffer::reset_mode reset_mode)
{
	assert(prepared_ && "HPPRenderContext not prepared for rendering, call prepare()");

	if (!frame_active_)
	{
		begin_frame();
	}

	if (!acquired_semaphore_)
	{
		throw std::runtime_error("Couldn't begin frame");
	}

	const auto &queue = device_.get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
	return active_frame().request_command_buffer(queue, reset_mode);
}

void render_context::submit(vkb::core::command_buffer &command_buffer)
{
	submit({&command_buffer});
}

void render_context::submit(const std::vector<vkb::core::command_buffer *> &command_buffers)
{
	assert(frame_active_ && "HPPRenderContext is inactive, cannot submit command buffer. Please call begin()");

	vk::Semaphore render_semaphore;

	if (swapchain_)
	{
		assert(acquired_semaphore_ && "We do not have acquired_semaphore, it was probably consumed?\n");
		render_semaphore = submit(queue_, command_buffers, acquired_semaphore_, vk::PipelineStageFlagBits::eColorAttachmentOutput);
	}
	else
	{
		submit(queue_, command_buffers);
	}

	end_frame(render_semaphore);
}

void render_context::begin_frame()
{
	// Only handle surface changes if a swapchain exists
	if (swapchain_)
	{
		handle_surface_changes();
	}

	assert(!frame_active_ && "Frame is still active, please call end_frame");

	auto &prev_frame = *frames_[active_frame_index_];

	// We will use the acquired semaphore in a different frame context,
	// so we need to hold ownership.
	acquired_semaphore_ = prev_frame.request_semaphore_with_ownership();

	if (swapchain_)
	{
		vk::Result result;
		try
		{
			std::tie(result, active_frame_index_) = swapchain_->acquire_next_image(acquired_semaphore_);
		}
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
			bool swapchain_updated = handle_surface_changes(result == vk::Result::eErrorOutOfDateKHR);

			if (swapchain_updated)
			{
				std::tie(result, active_frame_index_) = swapchain_->acquire_next_image(acquired_semaphore_);
			}
		}

		if (result != vk::Result::eSuccess)
		{
			prev_frame.reset();
			return;
		}
	}

	// Now the frame is active again
	frame_active_ = true;

	// Wait on all resource to be freed from the previous render to this frame
	wait_frame();
}

vk::Semaphore render_context::submit(const vkb::core::queue                        &queue,
                                       const std::vector<vkb::core::command_buffer *> &command_buffers,
                                       vk::Semaphore                                     wait_semaphore,
                                       vk::PipelineStageFlags                            wait_pipeline_stage)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const vkb::core::command_buffer *cmd_buf) { return cmd_buf->get_handle(); });

	vkb::rendering::render_frame &frame = active_frame();

	vk::Semaphore signal_semaphore = frame.request_semaphore();

	vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles, signal_semaphore);
	if (wait_semaphore)
	{
		submit_info.setWaitSemaphores(wait_semaphore);
		submit_info.pWaitDstStageMask = &wait_pipeline_stage;
	}

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);

	return signal_semaphore;
}

void render_context::submit(const vkb::core::queue &queue, const std::vector<vkb::core::command_buffer *> &command_buffers)
{
	std::vector<vk::CommandBuffer> cmd_buf_handles(command_buffers.size(), nullptr);
	std::transform(command_buffers.begin(), command_buffers.end(), cmd_buf_handles.begin(), [](const vkb::core::command_buffer *cmd_buf) { return cmd_buf->get_handle(); });

	vkb::rendering::render_frame &frame = active_frame();

	vk::SubmitInfo submit_info(nullptr, nullptr, cmd_buf_handles);

	vk::Fence fence = frame.request_fence();

	queue.get_handle().submit(submit_info, fence);
}

void render_context::wait_frame()
{
	active_frame().reset();
}

void render_context::end_frame(vk::Semaphore semaphore)
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");

	if (swapchain_)
	{
		vk::SwapchainKHR   vk_swapchain = swapchain_->handle();
		vk::PresentInfoKHR present_info(semaphore, vk_swapchain, active_frame_index_);

		vk::DisplayPresentInfoKHR disp_present_info;
		if (device_.is_extension_supported(VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) &&
		    window_.get_display_present_info(reinterpret_cast<VkDisplayPresentInfoKHR *>(&disp_present_info), surface_extent_.width, surface_extent_.height))
		{
			// Add display present info if supported and wanted
			present_info.pNext = &disp_present_info;
		}

		vk::Result result;
		try
		{
			result = queue_.present(present_info);
		}
		catch (vk::OutOfDateKHRError & /*err*/)
		{
			result = vk::Result::eErrorOutOfDateKHR;
		}

		if (result == vk::Result::eSuboptimalKHR || result == vk::Result::eErrorOutOfDateKHR)
		{
			handle_surface_changes();
		}
	}

	// Frame is not active anymore
	if (acquired_semaphore_)
	{
		release_owned_semaphore(acquired_semaphore_);
		acquired_semaphore_ = nullptr;
	}
	frame_active_ = false;
}

vk::Semaphore render_context::consume_acquired_semaphore()
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");
	return std::exchange(acquired_semaphore_, nullptr);
}

vkb::rendering::render_frame &render_context::active_frame()
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");
	return *frames_[active_frame_index_];
}

uint32_t render_context::active_frame_index()
{
	assert(frame_active_ && "Frame is not active, please call begin_frame");
	return active_frame_index_;
}

vkb::rendering::render_frame &render_context::last_rendered_frame()
{
	assert(!frame_active_ && "Frame is still active, please call end_frame");
	return *frames_[active_frame_index_];
}

vk::Semaphore render_context::request_semaphore()
{
	return active_frame().request_semaphore();
}

vk::Semaphore render_context::request_semaphore_with_ownership()
{
	return active_frame().request_semaphore_with_ownership();
}

void render_context::release_owned_semaphore(vk::Semaphore semaphore)
{
	active_frame().release_owned_semaphore(semaphore);
}

vkb::core::device &render_context::get_device()
{
	return device_;
}

void render_context::recreate_swapchain()
{
	device_.get_handle().waitIdle();
	device_.get_resource_cache().clear_framebuffers();

	vk::Extent2D swapchain_extent = swapchain_->extent();
	vk::Extent3D extent{swapchain_extent.width, swapchain_extent.height, 1};

	auto frame_it = frames_.begin();

	for (auto &image_handle : swapchain_->images())
	{
		vkb::core::image swapchain_image{device_, image_handle, extent, swapchain_->format(), swapchain_->usage()};
		auto                render_target = create_render_target_func_(std::move(swapchain_image));
		(*frame_it)->update_render_target(std::move(render_target));

		++frame_it;
	}
}

bool render_context::has_swapchain()
{
	return swapchain_ != nullptr;
}

vkb::core::swapchain const &render_context::swapchain() const
{
	assert(swapchain_ && "Swapchain is not valid");
	return *swapchain_;
}

vk::Extent2D const &render_context::surface_extent() const
{
	return surface_extent_;
}

uint32_t render_context::active_frame_index() const
{
	return active_frame_index_;
}

std::vector<std::unique_ptr<vkb::rendering::render_frame>> &render_context::render_frames()
{
	return frames_;
}

}        // namespace rendering
}        // namespace vkb
