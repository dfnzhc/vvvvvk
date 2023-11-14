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

#pragma once

#include "resource_caching.h"
#include <core/hpp_device.h>
#include <vulkan/vulkan_hash.hpp>

namespace std
{
template <typename Key, typename Value>
struct hash<std::map<Key, Value>>
{
	size_t operator()(std::map<Key, Value> const &bindings) const
	{
		size_t result = 0;
		vkb::hash_combine(result, bindings.size());
		for (auto &binding : bindings)
		{
			vkb::hash_combine(result, binding.first);
			vkb::hash_combine(result, binding.second);
		}
		return result;
	}
};

template <typename T>
struct hash<std::vector<T>>
{
	size_t operator()(std::vector<T> const &values) const
	{
		size_t result = 0;
		vkb::hash_combine(result, values.size());
		for (auto const &value : values)
		{
			vkb::hash_combine(result, value);
		}
		return result;
	}
};

template <>
struct hash<vkb::common::HPPLoadStoreInfo>
{
	size_t operator()(vkb::common::HPPLoadStoreInfo const &lsi) const
	{
		size_t result = 0;
		vkb::hash_combine(result, lsi.load_op);
		vkb::hash_combine(result, lsi.store_op);
		return result;
	}
};

template <typename T>
struct hash<vkb::core::vk_unit<T>>
{
	size_t operator()(const vkb::core::vk_unit<T> &vulkan_resource) const
	{
		return std::hash<T>()(vulkan_resource.get_handle());
	}
};

template <>
struct hash<vkb::core::descriptor_pool>
{
	size_t operator()(const vkb::core::descriptor_pool &descriptor_pool) const
	{
		return std::hash<vkb::DescriptorPool>()(reinterpret_cast<vkb::DescriptorPool const &>(descriptor_pool));
	}
};

template <>
struct hash<vkb::core::descriptor_set>
{
	size_t operator()(vkb::core::descriptor_set &descriptor_set) const
	{
		size_t result = 0;
		vkb::hash_combine(result, descriptor_set.layout());
		// descriptor_pool ?
		vkb::hash_combine(result, descriptor_set.buffer_infos());
		vkb::hash_combine(result, descriptor_set.image_infos());
		vkb::hash_combine(result, descriptor_set.handle());
		// write_descriptor_sets ?

		return result;
	}
};

template <>
struct hash<vkb::core::descriptor_set_layout>
{
	size_t operator()(const vkb::core::descriptor_set_layout &descriptor_set_layout) const
	{
		return std::hash<vkb::DescriptorSetLayout>()(reinterpret_cast<vkb::DescriptorSetLayout const &>(descriptor_set_layout));
	}
};

template <>
struct hash<vkb::core::image>
{
	size_t operator()(const vkb::core::image &image) const
	{
		size_t result = 0;
		vkb::hash_combine(result, image.memory());
		vkb::hash_combine(result, image.type());
		vkb::hash_combine(result, image.extent());
		vkb::hash_combine(result, image.format());
		vkb::hash_combine(result, image.usage());
		vkb::hash_combine(result, image.sample_count());
		vkb::hash_combine(result, image.tiling());
		vkb::hash_combine(result, image.subresource());
		vkb::hash_combine(result, image.array_layer_count());
		return result;
	}
};

template <>
struct hash<vkb::core::image_view>
{
	size_t operator()(const vkb::core::image_view &image_view) const
	{
		size_t result = std::hash<vkb::core::vk_unit<vk::ImageView>>()(image_view);
		vkb::hash_combine(result, image_view.image());
		vkb::hash_combine(result, image_view.format());
		vkb::hash_combine(result, image_view.subresource_range());
		return result;
	}
};

template <>
struct hash<vkb::core::render_pass>
{
	size_t operator()(const vkb::core::render_pass &render_pass) const
	{
		return std::hash<vkb::RenderPass>()(reinterpret_cast<vkb::RenderPass const &>(render_pass));
	}
};

template <>
struct hash<vkb::core::shader_module>
{
	size_t operator()(const vkb::core::shader_module &shader_module) const
	{
		return std::hash<vkb::ShaderModule>()(reinterpret_cast<vkb::ShaderModule const &>(shader_module));
	}
};

template <>
struct hash<vkb::core::shader_resource>
{
	size_t operator()(vkb::core::shader_resource const &shader_resource) const
	{
		size_t result = 0;
		vkb::hash_combine(result, shader_resource.stages);
		vkb::hash_combine(result, shader_resource.type);
		vkb::hash_combine(result, shader_resource.mode);
		vkb::hash_combine(result, shader_resource.set);
		vkb::hash_combine(result, shader_resource.binding);
		vkb::hash_combine(result, shader_resource.location);
		vkb::hash_combine(result, shader_resource.input_attachment_index);
		vkb::hash_combine(result, shader_resource.vec_size);
		vkb::hash_combine(result, shader_resource.columns);
		vkb::hash_combine(result, shader_resource.array_size);
		vkb::hash_combine(result, shader_resource.offset);
		vkb::hash_combine(result, shader_resource.size);
		vkb::hash_combine(result, shader_resource.constant_id);
		vkb::hash_combine(result, shader_resource.qualifiers);
		vkb::hash_combine(result, shader_resource.name);
		return result;
	}
};

template <>
struct hash<vkb::core::shader_source>
{
	size_t operator()(const vkb::core::shader_source &shader_source) const
	{
		return std::hash<vkb::ShaderSource>()(reinterpret_cast<vkb::ShaderSource const &>(shader_source));
	}
};

template <>
struct hash<vkb::core::shader_variant>
{
	size_t operator()(const vkb::core::shader_variant &shader_variant) const
	{
		return std::hash<vkb::ShaderVariant>()(reinterpret_cast<vkb::ShaderVariant const &>(shader_variant));
	}
};

template <>
struct hash<vkb::core::subpass_info>
{
	size_t operator()(vkb::core::subpass_info const &subpass_info) const
	{
		size_t result = 0;
		vkb::hash_combine(result, subpass_info.input_attachments);
		vkb::hash_combine(result, subpass_info.output_attachments);
		vkb::hash_combine(result, subpass_info.color_resolve_attachments);
		vkb::hash_combine(result, subpass_info.disable_depth_stencil_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_attachment);
		vkb::hash_combine(result, subpass_info.depth_stencil_resolve_mode);
		vkb::hash_combine(result, subpass_info.debug_name);
		return result;
	}
};

template <>
struct hash<vkb::rendering::attachment>
{
	size_t operator()(const vkb::rendering::attachment &attachment) const
	{
		size_t result = 0;
		vkb::hash_combine(result, attachment.format);
		vkb::hash_combine(result, attachment.samples);
		vkb::hash_combine(result, attachment.usage);
		vkb::hash_combine(result, attachment.initial_layout);
		return result;
	}
};

template <>
struct hash<vkb::rendering::HPPPipelineState>
{
	size_t operator()(const vkb::rendering::HPPPipelineState &pipeline_state) const
	{
		return std::hash<vkb::PipelineState>()(reinterpret_cast<vkb::PipelineState const &>(pipeline_state));
	}
};

template <>
struct hash<vkb::rendering::render_target>
{
	size_t operator()(const vkb::rendering::render_target &render_target) const
	{
		size_t result = 0;
		vkb::hash_combine(result, render_target.extent());
		for (auto const &view : render_target.views())
		{
			vkb::hash_combine(result, view);
		}
		for (auto const &attachment : render_target.attachments())
		{
			vkb::hash_combine(result, attachment);
		}
		for (auto const &input : render_target.input_attachments())
		{
			vkb::hash_combine(result, input);
		}
		for (auto const &output : render_target.output_attachments())
		{
			vkb::hash_combine(result, output);
		}
		return result;
	}
};

}        // namespace std

namespace vkb
{
/**
 * @brief facade helper functions and structs around the functions and structs in common/resource_caching, providing a vulkan.hpp-based interface
 */

namespace
{
template <class T, class... A>
struct HPPRecordHelper
{
	size_t record(resource_record & /*recorder*/, A &.../*args*/)
	{
		return 0;
	}

	void index(resource_record & /*recorder*/, size_t /*index*/, T & /*resource*/)
	{}
};

template <class... A>
struct HPPRecordHelper<vkb::core::shader_module, A...>
{
	size_t record(resource_record &recorder, A &...args)
	{
		return recorder.register_shader_module(args...);
	}

	void index(resource_record &recorder, size_t index, vkb::core::shader_module &shader_module)
	{
		recorder.set_shader_module(index, shader_module);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::pipeline_layout, A...>
{
	size_t record(resource_record &recorder, A &...args)
	{
		return recorder.register_pipeline_layout(args...);
	}

	void index(resource_record &recorder, size_t index, vkb::core::pipeline_layout &pipeline_layout)
	{
		recorder.set_pipeline_layout(index, pipeline_layout);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::render_pass, A...>
{
	size_t record(resource_record &recorder, A &...args)
	{
		return recorder.register_render_pass(args...);
	}

	void index(resource_record &recorder, size_t index, vkb::core::render_pass &render_pass)
	{
		recorder.set_render_pass(index, render_pass);
	}
};

template <class... A>
struct HPPRecordHelper<vkb::core::graphics_pipeline, A...>
{
	size_t record(resource_record &recorder, A &...args)
	{
		return recorder.register_graphics_pipeline(args...);
	}

	void index(resource_record &recorder, size_t index, vkb::core::graphics_pipeline &graphics_pipeline)
	{
		recorder.set_graphics_pipeline(index, graphics_pipeline);
	}
};
}        // namespace

template <class T, class... A>
T &request_resource(vkb::core::device &device, vkb::resource_record *recorder, std::unordered_map<size_t, T> &resources, A &...args)
{
	HPPRecordHelper<T, A...> record_helper;

	size_t hash{0U};
	hash_param(hash, args...);

	auto res_it = resources.find(hash);

	if (res_it != resources.end())
	{
		return res_it->second;
	}

	// If we do not have it already, create and cache it
	const char *res_type = typeid(T).name();
	size_t      res_id   = resources.size();

	LOGD("Building #{} cache object ({})", res_id, res_type);

// Only error handle in release
#ifndef DEBUG
	try
	{
#endif
		T resource(device, args...);

		auto res_ins_it = resources.emplace(hash, std::move(resource));

		if (!res_ins_it.second)
		{
			throw std::runtime_error{std::string{"Insertion error for #"} + std::to_string(res_id) + "cache object (" + res_type + ")"};
		}

		res_it = res_ins_it.first;

		if (recorder)
		{
			size_t index = record_helper.record(*recorder, args...);
			record_helper.index(*recorder, index, res_it->second);
		}
#ifndef DEBUG
	}
	catch (const std::exception &e)
	{
		LOGE("Creation error for #{} cache object ({})", res_id, res_type);
		throw e;
	}
#endif

	return res_it->second;
}
}        // namespace vkb