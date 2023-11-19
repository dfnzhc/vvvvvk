/**
 * @File Image.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "Image.hpp"
#include "Debug.hpp"
#include "Device.hpp"
#include "ImageView.hpp"

namespace {
inline vk::ImageType find_image_type(vk::Extent3D const& extent)
{
    uint32_t dim_num = !!extent.width + !!extent.height + (1 < extent.depth);
    switch (dim_num) {
        case 1:
            return vk::ImageType::e1D;
        case 2:
            return vk::ImageType::e2D;
        case 3:
            return vk::ImageType::e3D;
        default:
            throw std::runtime_error("No image type found.");
            return vk::ImageType();
    }
}
}        // namespace

vk_image::vk_image(vk_device& device,
                   const vk::Extent3D& extent,
                   vk::Format format,
                   vk::ImageUsageFlags image_usage,
                   VmaMemoryUsage memory_usage,
                   vk::SampleCountFlagBits sample_count,
                   const uint32_t mip_levels,
                   const uint32_t array_layers,
                   vk::ImageTiling tiling,
                   vk::ImageCreateFlags flags,
                   uint32_t num_queue_families,
                   const uint32_t* queue_families) :
    vk_unit{nullptr, &device},
    type{find_image_type(extent)},
    extent{extent},
    format{format},
    sample_count{sample_count},
    usage{image_usage},
    array_layer_count{array_layers},
    tiling{tiling}
{
    assert(0 < mip_levels && "图像至少有一个 mip 等级");
    assert(0 < array_layers && "图像至少有一个层级");

    subresource.mipLevel   = mip_levels;
    subresource.arrayLayer = array_layers;

    vk::ImageCreateInfo image_info(flags, type, format, extent, mip_levels, array_layers, sample_count, tiling,
                                   image_usage);

    if (num_queue_families != 0) {
        image_info.sharingMode           = vk::SharingMode::eConcurrent;
        image_info.queueFamilyIndexCount = num_queue_families;
        image_info.pQueueFamilyIndices   = queue_families;
    }

    VmaAllocationCreateInfo memory_info{};
    memory_info.usage = memory_usage;

    if (image_usage & vk::ImageUsageFlagBits::eTransientAttachment) {
        memory_info.preferredFlags = VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
    }

    auto result = vmaCreateImage(device.get_memory_allocator(),
                                 reinterpret_cast<const VkImageCreateInfo*>(&image_info),
                                 &memory_info,
                                 const_cast<VkImage*>(reinterpret_cast<const VkImage*>(&handle())),
                                 &memory,
                                 nullptr);

    if (result != VK_SUCCESS) {
        throw VulkanException{vk::Result(result), "创建图像失败"};
    }
}

vk_image::vk_image(vk_device& device,
                   vk::Image handle,
                   const vk::Extent3D& extent,
                   vk::Format format,
                   vk::ImageUsageFlags image_usage,
                   vk::SampleCountFlagBits sample_count) :
    vk_unit{handle, &device}, type{find_image_type(extent)}, extent{extent}, format{format}, sample_count{sample_count},
    usage{image_usage}
{
    subresource.mipLevel   = 1;
    subresource.arrayLayer = 1;
}

vk_image::vk_image(vk_image&& other) noexcept :
    vk_unit{std::move(other)},
    memory(std::exchange(other.memory, {})),
    type(std::exchange(other.type, {})),
    extent(std::exchange(other.extent, {})),
    format(std::exchange(other.format, {})),
    sample_count(std::exchange(other.sample_count, {})),
    usage(std::exchange(other.usage, {})),
    tiling(std::exchange(other.tiling, {})),
    subresource(std::exchange(other.subresource, {})),
    views(std::exchange(other.views, {})),
    mapped_data(std::exchange(other.mapped_data, {})),
    mapped(std::exchange(other.mapped, {}))
{
    // 更新所有引用这个图像的图像视图，防止空悬指针
    for (auto& view: views) {
        view->set_image(*this);
    }
}

vk_image::~vk_image()
{
    if (handle() && memory) {
        unmap();
        vmaDestroyImage(device().get_memory_allocator(), static_cast<VkImage>(handle()), memory);
    }
}

VmaAllocation vk_image::get_memory() const
{
    return memory;
}

uint8_t* vk_image::map()
{
    if (!mapped_data) {
        if (tiling != vk::ImageTiling::eLinear) {
            LOGW("映射的图像内存并不是线性的");
        }

        VK_CHECK(vmaMapMemory(device().get_memory_allocator(), memory, reinterpret_cast<void**>(&mapped_data)));
        mapped = true;
    }
    return mapped_data;
}

void vk_image::unmap()
{
    if (mapped) {
        vmaUnmapMemory(device().get_memory_allocator(), memory);
        mapped_data = nullptr;
        mapped      = false;
    }
}

vk::ImageType vk_image::get_type() const
{
    return type;
}

const vk::Extent3D& vk_image::get_extent() const
{
    return extent;
}

vk::Format vk_image::get_format() const
{
    return format;
}

vk::SampleCountFlagBits vk_image::get_sample_count() const
{
    return sample_count;
}

vk::ImageUsageFlags vk_image::get_usage() const
{
    return usage;
}

vk::ImageTiling vk_image::get_tiling() const
{
    return tiling;
}

vk::ImageSubresource vk_image::get_subresource() const
{
    return subresource;
}

uint32_t vk_image::get_array_layer_count() const
{
    return array_layer_count;
}

std::unordered_set<vk_image_view*>& vk_image::get_views()
{
    return views;
}
