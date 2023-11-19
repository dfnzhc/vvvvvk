/**
 * @File Deivce.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/18
 * @Brief 
 */

#include "Device.hpp"
#include "Debug.hpp"
#include "Queue.hpp"
#include "PhysicalDevice.hpp"
#include "CommandBufferPool.hpp"
#include "FencePool.hpp"
#include "Buffer.hpp"

#include <vulkan/vulkan.hpp>
#include "volk.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

vk_device::vk_device(vk_physical_device& gpu, vk::SurfaceKHR surface,
                     std::unique_ptr<vk_debug_utils>&& debug_utils,
                     std::unordered_map<const char*, bool> requested_extensions) :
    vk_unit{nullptr, this},
    debug_utils{std::move(debug_utils)},
    gpu{gpu}
{
    LOGI("选择的 GPU 设备是: {}", gpu.properties().deviceName.data());

    // 设备队列信息
    std::vector<vk::QueueFamilyProperties> queue_family_properties = gpu.queue_family_properties();
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos(queue_family_properties.size());
    std::vector<std::vector<float>>        queue_priorities(queue_family_properties.size());

    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index) {
        vk::QueueFamilyProperties const& queue_family_property = queue_family_properties[queue_family_index];
        queue_priorities[queue_family_index].resize(queue_family_property.queueCount, 0.5f);

        vk::DeviceQueueCreateInfo& queue_create_info = queue_create_infos[queue_family_index];

        queue_create_info.queueFamilyIndex = queue_family_index;
        queue_create_info.queueCount       = queue_family_property.queueCount;
        queue_create_info.pQueuePriorities = queue_priorities[queue_family_index].data();
    }

    device_extensions = gpu.handle().enumerateDeviceExtensionProperties();
    // Display supported extensions
    if (!device_extensions.empty()) {
        LOGD("启用的设备扩展:");
        for (auto& extension: device_extensions) {
            LOGD("  \t{}", extension.extensionName.data());
        }
    }

    bool can_get_memory_requirements = is_extension_supported(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
    bool has_dedicated_allocation    = is_extension_supported(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

    if (can_get_memory_requirements && has_dedicated_allocation) {
        enabled_extensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
        enabled_extensions.push_back(VK_KHR_DEDICATED_ALLOCATION_EXTENSION_NAME);

        LOGI("开启专属分配");
    }

    // query 功能
    if (is_extension_supported(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME)
        && is_extension_supported(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME)) {
        auto perf_counter_features     = gpu.request_extension_features<vk::PhysicalDevicePerformanceQueryFeaturesKHR>();
        auto host_query_reset_features = gpu.request_extension_features<vk::PhysicalDeviceHostQueryResetFeatures>();

        if (perf_counter_features.performanceCounterQueryPools
            && host_query_reset_features.hostQueryReset) {
            enabled_extensions.push_back(VK_KHR_PERFORMANCE_QUERY_EXTENSION_NAME);
            enabled_extensions.push_back(VK_EXT_HOST_QUERY_RESET_EXTENSION_NAME);

            LOGI("开启性能查询功能");
        }
    }

    // 创建前，先检查设备扩展是否都支持
    std::vector<const char*> unsupported_extensions{};

    for (auto& extension: requested_extensions) {
        if (is_extension_supported(extension.first)) {
            enabled_extensions.emplace_back(extension.first);
        } else {
            unsupported_extensions.emplace_back(extension.first);
        }
    }

    if (!enabled_extensions.empty()) {
        LOGI("设备支持以下扩展:");
        for (auto& extension: enabled_extensions) {
            LOGI("  \t{}", extension);
        }
    }

    if (!unsupported_extensions.empty()) {
        auto error = false;
        for (auto& extension: unsupported_extensions) {
            auto extension_is_optional = requested_extensions[extension];
            if (extension_is_optional) {
                LOGW("可选扩展 {} 不可用, 一些功能可能未开启", extension);
            } else {
                LOGE("必要的扩展 {} 不可用, 无法继续", extension);
                error = true;
            }
        }

        if (error) {
            throw VulkanException(vk::Result::eErrorExtensionNotPresent, "扩展不可用");
        }
    }

    std::vector<const char*> layers = {"VK_LAYER_KHRONOS_validation"};
    vk::DeviceCreateInfo     create_info({}, queue_create_infos, layers,
                                         enabled_extensions, &gpu.get_mutable_requested_features());

    create_info.pNext = gpu.get_extension_feature_chain();
    set_handle(gpu.handle().createDevice(create_info));

    VULKAN_HPP_DEFAULT_DISPATCHER.init(handle());

    queues.resize(queue_family_properties.size());
    for (uint32_t queue_family_index = 0U; queue_family_index < queue_family_properties.size(); ++queue_family_index) {
        vk::QueueFamilyProperties const& queue_family_property = queue_family_properties[queue_family_index];

        vk::Bool32    present_supported = gpu.handle().getSurfaceSupportKHR(queue_family_index, surface);
        for (uint32_t queue_index       = 0U; queue_index < queue_family_property.queueCount; ++queue_index) {
            queues[queue_family_index].emplace_back(*this, queue_family_index, queue_family_property,
                                                    present_supported, queue_index);
        }
    }

    // @formatter:off
    VmaVulkanFunctions vma_vulkan_func{};
    vma_vulkan_func.vkAllocateMemory                    = reinterpret_cast<PFN_vkAllocateMemory>(handle().getProcAddr("vkAllocateMemory"));
    vma_vulkan_func.vkBindBufferMemory                  = reinterpret_cast<PFN_vkBindBufferMemory>(handle().getProcAddr("vkBindBufferMemory"));
    vma_vulkan_func.vkBindImageMemory                   = reinterpret_cast<PFN_vkBindImageMemory>(handle().getProcAddr("vkBindImageMemory"));
    vma_vulkan_func.vkCreateBuffer                      = reinterpret_cast<PFN_vkCreateBuffer>(handle().getProcAddr("vkCreateBuffer"));
    vma_vulkan_func.vkCreateImage                       = reinterpret_cast<PFN_vkCreateImage>(handle().getProcAddr("vkCreateImage"));
    vma_vulkan_func.vkDestroyBuffer                     = reinterpret_cast<PFN_vkDestroyBuffer>(handle().getProcAddr("vkDestroyBuffer"));
    vma_vulkan_func.vkDestroyImage                      = reinterpret_cast<PFN_vkDestroyImage>(handle().getProcAddr("vkDestroyImage"));
    vma_vulkan_func.vkFlushMappedMemoryRanges           = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(handle().getProcAddr("vkFlushMappedMemoryRanges"));
    vma_vulkan_func.vkFreeMemory                        = reinterpret_cast<PFN_vkFreeMemory>(handle().getProcAddr("vkFreeMemory"));
    vma_vulkan_func.vkGetBufferMemoryRequirements       = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(handle().getProcAddr("vkGetBufferMemoryRequirements"));
    vma_vulkan_func.vkGetImageMemoryRequirements        = reinterpret_cast<PFN_vkGetImageMemoryRequirements>(handle().getProcAddr("vkGetImageMemoryRequirements"));
    vma_vulkan_func.vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(handle().getProcAddr("vkGetPhysicalDeviceMemoryProperties"));
    vma_vulkan_func.vkGetPhysicalDeviceProperties       = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(handle().getProcAddr("vkGetPhysicalDeviceProperties"));
    vma_vulkan_func.vkInvalidateMappedMemoryRanges      = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(handle().getProcAddr("vkInvalidateMappedMemoryRanges"));
    vma_vulkan_func.vkMapMemory                         = reinterpret_cast<PFN_vkMapMemory>(handle().getProcAddr("vkMapMemory"));
    vma_vulkan_func.vkUnmapMemory                       = reinterpret_cast<PFN_vkUnmapMemory>(handle().getProcAddr("vkUnmapMemory"));
    vma_vulkan_func.vkCmdCopyBuffer                     = reinterpret_cast<PFN_vkCmdCopyBuffer>(handle().getProcAddr("vkCmdCopyBuffer"));
    vma_vulkan_func.vkGetImageMemoryRequirements2KHR    = reinterpret_cast<PFN_vkGetImageMemoryRequirements2KHR>(handle().getProcAddr("vkGetImageMemoryRequirements2KHR"));
    vma_vulkan_func.vkGetBufferMemoryRequirements2KHR   = reinterpret_cast<PFN_vkGetBufferMemoryRequirements2KHR>(handle().getProcAddr("vkGetBufferMemoryRequirements2KHR"));
    // @formatter:on

    VmaAllocatorCreateInfo allocator_info{};
    allocator_info.physicalDevice = static_cast<VkPhysicalDevice>(gpu.handle());
    allocator_info.device         = static_cast<VkDevice>(handle());
    allocator_info.instance       = static_cast<VkInstance>(gpu.instance());

    if (can_get_memory_requirements && has_dedicated_allocation) {
        allocator_info.flags |= VMA_ALLOCATOR_CREATE_KHR_DEDICATED_ALLOCATION_BIT;
        vma_vulkan_func.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
        vma_vulkan_func.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2KHR;
    }

    vma_vulkan_func.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vma_vulkan_func.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    if (is_extension_supported(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME) &&
        is_enabled(VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
        allocator_info.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    }

    allocator_info.pVulkanFunctions = &vma_vulkan_func;
    VK_CHECK(vmaCreateAllocator(&allocator_info, &memory_allocator));


    command_pool = std::make_unique<vk_command_pool>(*this, get_queue_by_flags(vk::QueueFlagBits::eGraphics |
                                                                               vk::QueueFlagBits::eCompute,
                                                                               0).get_family_index());

    fence_pool = std::make_unique<vk_fence_pool>(*this);
}

vk_device::~vk_device()
{
    command_pool.reset();
    fence_pool.reset();

    if (memory_allocator != VK_NULL_HANDLE) {
//        vm stats;
//        vmaCalculateStats(memory_allocator, &stats);
//
//        LOGI("Total device memory leaked: {} bytes.", stats.total.usedBytes);

        vmaDestroyAllocator(memory_allocator);
    }

    if (handle()) {
        handle().destroy();
        handle() = VK_NULL_HANDLE;
    }
}

const vk_physical_device& vk_device::get_gpu() const
{
    return gpu;
}

const VmaAllocator& vk_device::get_memory_allocator() const
{
    return memory_allocator;
}

const vk_debug_utils& vk_device::get_debug_utils() const
{
    return *debug_utils;
}

bool vk_device::is_extension_supported(const std::string& extension) const
{
    return std::find_if(device_extensions.begin(), device_extensions.end(),
                        [extension](auto& device_extension) {
                            return std::strcmp(device_extension.extensionName, extension.c_str()) == 0;
                        }) != device_extensions.end();
}

bool vk_device::is_enabled(const std::string& extension) const
{
    return std::find_if(enabled_extensions.begin(), enabled_extensions.end(),
                        [extension](const char* enabled_extension) { return extension == enabled_extension; }) !=
           enabled_extensions.end();
}

const vk_queue& vk_device::get_queue(uint32_t queue_family_index, uint32_t queue_index) const
{
    return queues[queue_family_index][queue_index];
}

const vk_queue& vk_device::get_queue_by_flags(vk::QueueFlags required_queue_flags, uint32_t queue_index) const
{
    for (size_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index) {
        const auto& first_queue = queues[queue_family_index][0];

        vk::QueueFlags queue_flags = first_queue.get_properties().queueFlags;
        uint32_t       queue_count = first_queue.get_properties().queueCount;

        if (((queue_flags & required_queue_flags) == required_queue_flags) && queue_index < queue_count) {
            return queues[queue_family_index][queue_index];
        }
    }

    throw std::runtime_error("没有这样的设备队列");
}

const vk_queue& vk_device::get_queue_by_present(uint32_t queue_index) const
{
    for (uint32_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index) {
        const auto& first_queue = queues[queue_family_index][0];

        uint32_t queue_count = first_queue.get_properties().queueCount;

        if (first_queue.support_present() && queue_index < queue_count) {
            return queues[queue_family_index][queue_index];
        }
    }

    throw std::runtime_error("没有这样的设备队列");
}

uint32_t vk_device::get_queue_family_index(vk::QueueFlagBits queue_flag) const
{
    const auto& queue_family_properties = gpu.queue_family_properties();

    if (queue_flag & vk::QueueFlagBits::eCompute) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
            if ((queue_family_properties[i].queueFlags & queue_flag) &&
                !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics)) {
                return i;
                break;
            }
        }
    }

    if (queue_flag & vk::QueueFlagBits::eTransfer) {
        for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
            if ((queue_family_properties[i].queueFlags & queue_flag) &&
                !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eGraphics) &&
                !(queue_family_properties[i].queueFlags & vk::QueueFlagBits::eCompute)) {
                return i;
                break;
            }
        }
    }

    for (uint32_t i = 0; i < static_cast<uint32_t>(queue_family_properties.size()); i++) {
        if (queue_family_properties[i].queueFlags & queue_flag) {
            return i;
            break;
        }
    }

    throw std::runtime_error("没有符合索引的设备队列");
}

const vk_queue& vk_device::get_suitable_graphics_queue() const
{
    for (size_t queue_family_index = 0U; queue_family_index < queues.size(); ++queue_family_index) {
        const auto& first_queue = queues[queue_family_index][0];

        uint32_t queue_count = first_queue.get_properties().queueCount;

        if (first_queue.support_present() && 0 < queue_count) {
            return queues[queue_family_index][0];
        }
    }

    return get_queue_by_flags(vk::QueueFlagBits::eGraphics, 0);
}

void vk_device::wait_idle() const
{
    handle().waitIdle();
}

std::pair<vk::Buffer, vk::DeviceMemory>
vk_device::create_buffer(vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::DeviceSize size,
                         void* data) const
{
    vk::Device device = handle();

    // Create the buffer handle
    vk::BufferCreateInfo buffer_create_info({}, size, usage, vk::SharingMode::eExclusive);
    vk::Buffer           buffer = device.createBuffer(buffer_create_info);

    // Create the memory backing up the buffer handle
    vk::MemoryRequirements memory_requirements = device.getBufferMemoryRequirements(buffer);
    vk::MemoryAllocateInfo memory_allocation(memory_requirements.size,
                                             get_gpu().memory_type(memory_requirements.memoryTypeBits, properties));
    vk::DeviceMemory       memory              = device.allocateMemory(memory_allocation);

    // If a pointer to the buffer data has been passed, map the buffer and copy over the
    if (data != nullptr) {
        void* mapped = device.mapMemory(memory, 0, size);
        memcpy(mapped, data, static_cast<size_t>(size));
        // If host coherency hasn't been requested, do a manual flush to make writes visible
        if (!(properties & vk::MemoryPropertyFlagBits::eHostCoherent)) {
            vk::MappedMemoryRange mapped_range(memory, 0, size);
            device.flushMappedMemoryRanges(mapped_range);
        }
        device.unmapMemory(memory);
    }

    // Attach the memory to the buffer object
    device.bindBufferMemory(buffer, memory, 0);

    return std::make_pair(buffer, memory);
}

std::pair<vk::Image, vk::DeviceMemory>
vk_device::create_image(vk::Format format, const vk::Extent2D& extent, uint32_t mip_levels, vk::ImageUsageFlags usage,
                        vk::MemoryPropertyFlags properties) const
{
    vk::Device device = handle();

    vk::ImageCreateInfo image_create_info({}, vk::ImageType::e2D, format, vk::Extent3D(extent, 1), mip_levels, 1,
                                          vk::SampleCountFlagBits::e1, vk::ImageTiling::eOptimal, usage);

    vk::Image image = device.createImage(image_create_info);

    vk::MemoryRequirements memory_requirements = device.getImageMemoryRequirements(image);

    vk::MemoryAllocateInfo memory_allocation(memory_requirements.size,
                                             get_gpu().memory_type(memory_requirements.memoryTypeBits, properties));
    vk::DeviceMemory       memory = device.allocateMemory(memory_allocation);
    device.bindImageMemory(image, memory, 0);

    return std::make_pair(image, memory);
}

void vk_device::copy_buffer(vk_buffer& src, vk_buffer& dst, vk::Queue queue, vk::BufferCopy* copy_region) const
{
    assert(dst.get_size() <= src.get_size());
    assert(src.handle());

    vk::CommandBuffer command_buffer = create_command_buffer(vk::CommandBufferLevel::ePrimary, true);

    vk::BufferCopy buffer_copy{};
    if (copy_region) {
        buffer_copy = *copy_region;
    } else {
        buffer_copy.size = src.get_size();
    }

    command_buffer.copyBuffer(src.handle(), dst.handle(), buffer_copy);

    flush_command_buffer(command_buffer, queue);
}

vk_command_pool& vk_device::get_command_pool()
{
    return *command_pool;
}

vk::CommandBuffer vk_device::create_command_buffer(vk::CommandBufferLevel level, bool begin) const
{
    assert(command_pool && "No command pool exists in the device");

    vk::CommandBuffer command_buffer = handle().allocateCommandBuffers(
        {command_pool->get_handle(), level, 1}).front();

    // If requested, also start recording for the new command buffer
    if (begin) {
        command_buffer.begin(vk::CommandBufferBeginInfo());
    }

    return command_buffer;
}

void vk_device::flush_command_buffer(vk::CommandBuffer command_buffer, vk::Queue queue, bool free,
                                     vk::Semaphore signalSemaphore) const
{
    if (!command_buffer) {
        return;
    }

    command_buffer.end();

    vk::SubmitInfo submit_info({}, {}, command_buffer);
    if (signalSemaphore) {
        submit_info.setSignalSemaphores(signalSemaphore);
    }

    // Create fence to ensure that the command buffer has finished executing
    vk::Fence fence = handle().createFence({});

    // Submit to the queue
    queue.submit(submit_info, fence);

    // Wait for the fence to signal that command buffer has finished executing
    vk::Result result = handle().waitForFences(fence, true, DEFAULT_FENCE_TIMEOUT);
    if (result != vk::Result::eSuccess) {
        LOGE("Detected Vulkan error: {}", vk::to_string(result));
        abort();
    }

    handle().destroyFence(fence);

    if (command_pool && free) {
        handle().freeCommandBuffers(command_pool->get_handle(), command_buffer);
    }
}

vk_fence_pool& vk_device::get_fence_pool()
{
    return *fence_pool;
}
