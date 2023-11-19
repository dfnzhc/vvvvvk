/**
 * @File Renderpass.cpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#include "Renderpass.hpp"
#include "Debug.hpp"
#include "Device.hpp"
#include "RenderTarget.hpp"
#include "Helpers.hpp"

namespace {

inline void set_pointer_next(vk::SubpassDescription& subpass_description,
                             vk::SubpassDescriptionDepthStencilResolveKHR& depth_resolve,
                             vk::AttachmentReference& depth_resolve_attachment)
{
    // vk::SubpassDescription cannot have pNext point to a vk::SubpassDescriptionDepthStencilResolveKHR containing a vk::AttachmentReference
}

inline void set_pointer_next(vk::SubpassDescription2KHR& subpass_description,
                             vk::SubpassDescriptionDepthStencilResolveKHR& depth_resolve,
                             vk::AttachmentReference2KHR& depth_resolve_attachment)
{
    depth_resolve.pDepthStencilResolveAttachment = &depth_resolve_attachment;
    subpass_description.pNext                    = &depth_resolve;
}

inline const vk::AttachmentReference2KHR* get_depth_resolve_reference(const vk::SubpassDescription& subpass_description)
{
    return nullptr;
}

inline const vk::AttachmentReference2KHR*
get_depth_resolve_reference(const vk::SubpassDescription2KHR& subpass_description)
{
    auto description_depth_resolve = static_cast<const vk::SubpassDescriptionDepthStencilResolveKHR*>(subpass_description.pNext);

    const vk::AttachmentReference2KHR* depth_resolve_attachment = nullptr;
    if (description_depth_resolve) {
        depth_resolve_attachment = description_depth_resolve->pDepthStencilResolveAttachment;
    }

    return depth_resolve_attachment;
}

inline vk::Result create_vk_renderpass(vk::Device device, vk::RenderPassCreateInfo& create_info, vk::RenderPass* handle)
{
    return device.createRenderPass(&create_info, nullptr, handle);
}

inline vk::Result
create_vk_renderpass(vk::Device device, vk::RenderPassCreateInfo2KHR& create_info, vk::RenderPass* handle)
{
    return device.createRenderPass2KHR(&create_info, nullptr, handle);
}
}        // namespace

template<typename T>
std::vector<T> get_attachment_descriptions(const std::vector<rt_attachment>& attachments,
                                           const std::vector<LoadStoreInfo>& load_store_infos)
{
    std::vector<T> attachment_descriptions;

    for (size_t i = 0U; i < attachments.size(); ++i) {
        T attachment{};

        attachment.format        = attachments[i].format;
        attachment.samples       = attachments[i].samples;
        attachment.initialLayout = attachments[i].initial_layout;
        attachment.finalLayout =
            is_depth_format(attachment.format) ? vk::ImageLayout::eDepthStencilAttachmentOptimal
                                               : vk::ImageLayout::eColorAttachmentOptimal;

        if (i < load_store_infos.size()) {
            attachment.loadOp         = load_store_infos[i].load_op;
            attachment.storeOp        = load_store_infos[i].store_op;
            attachment.stencilLoadOp  = load_store_infos[i].load_op;
            attachment.stencilStoreOp = load_store_infos[i].store_op;
        }

        attachment_descriptions.push_back(std::move(attachment));
    }

    return attachment_descriptions;
}

template<typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference>
void set_attachment_layouts(std::vector<T_SubpassDescription>& subpass_descriptions,
                            std::vector<T_AttachmentDescription>& attachment_descriptions)
{
    // Make the initial layout same as in the first subpass using that attachment
    for (auto& subpass: subpass_descriptions) {
        for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
            auto& reference = subpass.pColorAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k) {
            auto& reference = subpass.pInputAttachments[k];
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        if (subpass.pDepthStencilAttachment) {
            auto& reference = *subpass.pDepthStencilAttachment;
            // Set it only if not defined yet
            if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                attachment_descriptions[reference.attachment].initialLayout = reference.layout;
            }
        }

        if (subpass.pResolveAttachments) {
            for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
                auto& reference = subpass.pResolveAttachments[k];
                // Set it only if not defined yet
                if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                    attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                }
            }
        }

        if (const auto depth_resolve = get_depth_resolve_reference(subpass)) {
            // Set it only if not defined yet
            if (attachment_descriptions[depth_resolve->attachment].initialLayout == vk::ImageLayout::eUndefined) {
                attachment_descriptions[depth_resolve->attachment].initialLayout = depth_resolve->layout;
            }
        }
    }

    // Make the final layout same as the last subpass layout
    {
        auto& subpass = subpass_descriptions.back();

        for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
            const auto& reference = subpass.pColorAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }

        for (size_t k = 0U; k < subpass.inputAttachmentCount; ++k) {
            const auto& reference = subpass.pInputAttachments[k];

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;

            // Do not use depth attachment if used as input
            if (is_depth_format(attachment_descriptions[reference.attachment].format)) {
                subpass.pDepthStencilAttachment = nullptr;
            }
        }

        if (subpass.pDepthStencilAttachment) {
            const auto& reference = *subpass.pDepthStencilAttachment;

            attachment_descriptions[reference.attachment].finalLayout = reference.layout;
        }

        if (subpass.pResolveAttachments) {
            for (size_t k = 0U; k < subpass.colorAttachmentCount; ++k) {
                const auto& reference = subpass.pResolveAttachments[k];

                attachment_descriptions[reference.attachment].finalLayout = reference.layout;
            }
        }

        if (const auto depth_resolve = get_depth_resolve_reference(subpass)) {
            attachment_descriptions[depth_resolve->attachment].finalLayout = depth_resolve->layout;
        }
    }
}

template<typename T>
std::vector<T> get_subpass_dependencies(const size_t subpass_count)
{
    std::vector<T> dependencies(subpass_count - 1);

    if (subpass_count > 1) {
        for (uint32_t i = 0; i < to_u32(dependencies.size()); ++i) {
            // Transition input attachments from color attachment to shader read
            dependencies[i].srcSubpass      = i;
            dependencies[i].dstSubpass      = i + 1;
            dependencies[i].srcStageMask    = vk::PipelineStageFlagBits::eColorAttachmentOutput;
            dependencies[i].dstStageMask    = vk::PipelineStageFlagBits::eFragmentShader;
            dependencies[i].srcAccessMask   = vk::AccessFlagBits::eColorAttachmentWrite;
            dependencies[i].dstAccessMask   = vk::AccessFlagBits::eInputAttachmentRead;
            dependencies[i].dependencyFlags = vk::DependencyFlagBits::eByRegion;
        }
    }

    return dependencies;
}

template<typename T>
T get_attachment_reference(const uint32_t attachment, const vk::ImageLayout layout)
{
    T reference{};

    reference.attachment = attachment;
    reference.layout     = layout;
    return reference;
}

template<typename T_SubpassDescription, typename T_AttachmentDescription, typename T_AttachmentReference, typename T_SubpassDependency, typename T_RenderPassCreateInfo>
void vk_renderpass::create_renderpass(const std::vector<rt_attachment>& attachments,
                                      const std::vector<LoadStoreInfo>& load_store_infos,
                                      const std::vector<SubpassInfo>& subpasses)
{
    auto attachment_descriptions = get_attachment_descriptions<T_AttachmentDescription>(attachments, load_store_infos);

    // Store attachments for every subpass
    std::vector<std::vector<T_AttachmentReference>> input_attachments{subpass_count};
    std::vector<std::vector<T_AttachmentReference>> color_attachments{subpass_count};
    std::vector<std::vector<T_AttachmentReference>> depth_stencil_attachments{subpass_count};
    std::vector<std::vector<T_AttachmentReference>> color_resolve_attachments{subpass_count};
    std::vector<std::vector<T_AttachmentReference>> depth_resolve_attachments{subpass_count};

    std::string new_debug_name{};
    const bool  needs_debug_name = debug_name().empty();
    if (needs_debug_name) {
        new_debug_name = fmt::format("RP with {} subpasses:\n", subpasses.size());
    }

    for (size_t i = 0; i < subpasses.size(); ++i) {
        auto& subpass = subpasses[i];

        if (needs_debug_name) {
            new_debug_name += fmt::format("\t[{}]: {}\n", i, subpass.debug_name);
        }

        // Fill color attachments references
        for (auto o_attachment: subpass.output_attachments) {
            auto initial_layout = attachments[o_attachment].initial_layout == vk::ImageLayout::eUndefined
                                  ? vk::ImageLayout::eColorAttachmentOptimal : attachments[o_attachment].initial_layout;
            auto& description = attachment_descriptions[o_attachment];
            if (!is_depth_format(description.format)) {
                color_attachments[i]
                    .push_back(get_attachment_reference<T_AttachmentReference>(o_attachment, initial_layout));
            }
        }

        // Fill input attachments references
        for (auto i_attachment: subpass.input_attachments) {
            auto default_layout = is_depth_format(attachment_descriptions[i_attachment].format)
                                  ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                  : vk::ImageLayout::eShaderReadOnlyOptimal;
            auto initial_layout =
                     attachments[i_attachment].initial_layout == vk::ImageLayout::eUndefined ? default_layout
                                                                                             : attachments[i_attachment].initial_layout;
            input_attachments[i].push_back(
                get_attachment_reference<T_AttachmentReference>(i_attachment, initial_layout));
        }

        for (auto r_attachment: subpass.color_resolve_attachments) {
            auto initial_layout = attachments[r_attachment].initial_layout == vk::ImageLayout::eUndefined
                                  ? vk::ImageLayout::eColorAttachmentOptimal : attachments[r_attachment].initial_layout;
            color_resolve_attachments[i].push_back(
                get_attachment_reference<T_AttachmentReference>(r_attachment, initial_layout));
        }

        if (!subpass.disable_depth_stencil_attachment) {
            // Assumption: depth stencil attachment appears in the list before any depth stencil resolve attachment
            auto it = find_if(attachments.begin(), attachments.end(),
                              [](const rt_attachment attachment) { return is_depth_format(attachment.format); });
            if (it != attachments.end()) {
                auto i_depth_stencil = to_u32(std::distance(attachments.begin(), it));
                auto initial_layout  = it->initial_layout == vk::ImageLayout::eUndefined
                                       ? vk::ImageLayout::eDepthStencilReadOnlyOptimal : it->initial_layout;
                depth_stencil_attachments[i].push_back(
                    get_attachment_reference<T_AttachmentReference>(i_depth_stencil, initial_layout));

                if (subpass.depth_stencil_resolve_mode != vk::ResolveModeFlagBits::eNone) {
                    auto i_depth_stencil_resolve = subpass.depth_stencil_resolve_attachment;
                    initial_layout = attachments[i_depth_stencil_resolve].initial_layout == vk::ImageLayout::eUndefined
                                     ? vk::ImageLayout::eDepthStencilReadOnlyOptimal
                                     : attachments[i_depth_stencil_resolve].initial_layout;
                    depth_resolve_attachments[i].push_back(
                        get_attachment_reference<T_AttachmentReference>(i_depth_stencil_resolve, initial_layout));
                }
            }
        }
    }

    std::vector<T_SubpassDescription> subpass_descriptions;
    subpass_descriptions.reserve(subpass_count);
    vk::SubpassDescriptionDepthStencilResolveKHR depth_resolve{};

    for (size_t i = 0; i < subpasses.size(); ++i) {
        auto& subpass = subpasses[i];

        T_SubpassDescription subpass_description{};
        subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;

        subpass_description.pInputAttachments    = input_attachments[i].empty() ? nullptr : input_attachments[i].data();
        subpass_description.inputAttachmentCount = to_u32(input_attachments[i].size());

        subpass_description.pColorAttachments    = color_attachments[i].empty() ? nullptr : color_attachments[i].data();
        subpass_description.colorAttachmentCount = to_u32(color_attachments[i].size());

        subpass_description.pResolveAttachments = color_resolve_attachments[i].empty() ? nullptr
                                                                                       : color_resolve_attachments[i].data();

        subpass_description.pDepthStencilAttachment = nullptr;
        if (!depth_stencil_attachments[i].empty()) {
            subpass_description.pDepthStencilAttachment = depth_stencil_attachments[i].data();

            if (!depth_resolve_attachments[i].empty()) {
                depth_resolve.depthResolveMode = subpass.depth_stencil_resolve_mode;
                set_pointer_next(subpass_description, depth_resolve, depth_resolve_attachments[i][0]);

                auto& reference = depth_resolve_attachments[i][0];
                // Set it only if not defined yet
                if (attachment_descriptions[reference.attachment].initialLayout == vk::ImageLayout::eUndefined) {
                    attachment_descriptions[reference.attachment].initialLayout = reference.layout;
                }
            }
        }

        subpass_descriptions.push_back(subpass_description);
    }

    // Default subpass
    if (subpasses.empty()) {
        T_SubpassDescription subpass_description{};

        subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
        uint32_t default_depth_stencil_attachment{VK_ATTACHMENT_UNUSED};

        for (uint32_t k = 0U; k < to_u32(attachment_descriptions.size()); ++k) {
            if (is_depth_format(attachments[k].format)) {
                if (default_depth_stencil_attachment == VK_ATTACHMENT_UNUSED) {
                    default_depth_stencil_attachment = k;
                }
                continue;
            }

            color_attachments[0]
                .push_back(get_attachment_reference<T_AttachmentReference>(k, vk::ImageLayout::eGeneral));
        }

        subpass_description.pColorAttachments = color_attachments[0].data();

        if (default_depth_stencil_attachment != VK_ATTACHMENT_UNUSED) {
            depth_stencil_attachments[0].push_back(
                get_attachment_reference<T_AttachmentReference>(default_depth_stencil_attachment,
                                                                vk::ImageLayout::eDepthStencilReadOnlyOptimal));

            subpass_description.pDepthStencilAttachment = depth_stencil_attachments[0].data();
        }

        subpass_descriptions.push_back(subpass_description);
    }

    set_attachment_layouts<T_SubpassDescription, T_AttachmentDescription, T_AttachmentReference>(subpass_descriptions,
                                                                                                 attachment_descriptions);

    color_output_count.reserve(subpass_count);
    for (size_t i = 0; i < subpass_count; i++) {
        color_output_count.push_back(to_u32(color_attachments[i].size()));
    }

    const auto& subpass_dependencies = get_subpass_dependencies<T_SubpassDependency>(subpass_count);

    T_RenderPassCreateInfo create_info{};
    create_info.attachmentCount = to_u32(attachment_descriptions.size());
    create_info.pAttachments    = attachment_descriptions.data();
    create_info.subpassCount    = to_u32(subpass_descriptions.size());
    create_info.pSubpasses      = subpass_descriptions.data();
    create_info.dependencyCount = to_u32(subpass_dependencies.size());
    create_info.pDependencies   = subpass_dependencies.data();

    VK_CHECK(create_vk_renderpass(device().handle(), create_info, &handle()));

    if (needs_debug_name) {
        set_debug_name(new_debug_name);
    }
}

vk_renderpass::vk_renderpass(vk_device& device,
                             const std::vector<rt_attachment>& attachments,
                             const std::vector<LoadStoreInfo>& load_store_infos,
                             const std::vector<SubpassInfo>& subpasses) :
    vk_unit{VK_NULL_HANDLE, &device},
    subpass_count{std::max<size_t>(1, subpasses.size())},        // At least 1 subpass
    color_output_count{}
{
    if (device.is_enabled(VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME)) {
        create_renderpass<vk::SubpassDescription2KHR,
            vk::AttachmentDescription2KHR,
            vk::AttachmentReference2KHR,
            vk::SubpassDependency2KHR,
            vk::RenderPassCreateInfo2KHR>(
            attachments, load_store_infos, subpasses);
    } else {
        create_renderpass<vk::SubpassDescription,
            vk::AttachmentDescription,
            vk::AttachmentReference,
            vk::SubpassDependency,
            vk::RenderPassCreateInfo>(
            attachments, load_store_infos, subpasses);
    }
}

vk_renderpass::vk_renderpass(vk_renderpass&& other) :
    vk_unit{std::move(other)},
    subpass_count{other.subpass_count},
    color_output_count{other.color_output_count}
{
    other.handle() = VK_NULL_HANDLE;
}

vk_renderpass::~vk_renderpass()
{
    // Destroy render pass
    if (handle() != VK_NULL_HANDLE) {
        device().handle().destroyRenderPass(handle(), nullptr);
    }
}

const uint32_t vk_renderpass::get_color_output_count(uint32_t subpass_index) const
{
    return color_output_count[subpass_index];
}

const vk::Extent2D vk_renderpass::get_render_area_granularity() const
{
    vk::Extent2D render_area_granularity = {};
    device().handle().getRenderAreaGranularity(handle(), &render_area_granularity);

    return render_area_granularity;
}