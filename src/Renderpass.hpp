/**
 * @File Renderpass.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "VkUnit.hpp"

class vk_device;

struct rt_attachment;

struct SubpassInfo
{
    std::vector<uint32_t> input_attachments;
    std::vector<uint32_t> output_attachments;
    std::vector<uint32_t> color_resolve_attachments;

    bool                    disable_depth_stencil_attachment;
    uint32_t                depth_stencil_resolve_attachment;
    vk::ResolveModeFlagBits depth_stencil_resolve_mode;

    std::string debug_name;
};

class vk_renderpass : public vk_unit<vk::RenderPass>
{
public:
    vk_renderpass(vk_device& device,
                  const std::vector<rt_attachment>& attachments,
                  const std::vector<LoadStoreInfo>& load_store_infos,
                  const std::vector<SubpassInfo>& subpasses);

    vk_renderpass(vk_renderpass&& other);

    ~vk_renderpass();

    vk_renderpass(const vk_renderpass&) = delete;

    vk_renderpass& operator=(const vk_renderpass&) = delete;
    vk_renderpass& operator=(vk_renderpass&&) = delete;

    const uint32_t get_color_output_count(uint32_t subpass_index) const;
    const vk::Extent2D get_render_area_granularity() const;

private:
    size_t subpass_count;

    template<typename T_SubpassDescription,
        typename T_AttachmentDescription,
        typename T_AttachmentReference,
        typename T_SubpassDependency,
        typename T_RenderPassCreateInfo>
    void create_renderpass(const std::vector<rt_attachment>& attachments,
                           const std::vector<LoadStoreInfo>& load_store_infos,
                           const std::vector<SubpassInfo>& subpasses);

    std::vector<uint32_t> color_output_count;
};