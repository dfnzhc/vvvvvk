/**
 * @File RenderTarget.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "VkCommon.hpp"
#include "Image.hpp"

struct rt_attachment
{
    rt_attachment() = default;

    rt_attachment(vk::Format format, vk::SampleCountFlagBits samples, vk::ImageUsageFlags usage) :
        format{format},
        samples{samples},
        usage{usage} {}

    vk::Format              format         = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples        = vk::SampleCountFlagBits::e1;
    vk::ImageUsageFlags     usage          = vk::ImageUsageFlagBits::eSampled;
    vk::ImageLayout         initial_layout = vk::ImageLayout::eUndefined;
};

class vk_render_target
{
public:
    /**
     * @brief 从图像创建渲染目标的方法
     */
    using CreateFunc = std::function<std::unique_ptr<vk_render_target>(vk_image&&)>;

    static const CreateFunc DEFAULT_CREATE_FUNC;

    explicit vk_render_target(std::vector<vk_image>&& images);
    explicit vk_render_target(std::vector<vk_image_view>&& image_views);

    vk_render_target(const vk_render_target&) = delete;
    vk_render_target(vk_render_target&&) = delete;

    vk_render_target& operator=(const vk_render_target& other) noexcept = delete;
    vk_render_target& operator=(vk_render_target&& other) noexcept = delete;

    // @formatter:off
    const vk::Extent2D&                 get_extent() const;
    const std::vector<vk_image_view>&   get_views() const;
    const std::vector<rt_attachment>&   get_attachments() const;
    // @formatter:on

    /**
     * @brief 设置并覆盖当前的输入附件，应该在渲染通道和子通道开始(begin)之前设置
     * @param input 一组作为输入的附件引用索引
     */
    // @formatter:off
    void                            set_input_attachments(std::vector<uint32_t>& input);
    const std::vector<uint32_t>&    get_input_attachments() const;
    // @formatter:on

    /**
     * @brief 设置并覆盖当前的输出附件，应该在渲染通道和子通道开始(begin)之前设置
     * @param input 一组作为输出的附件引用索引
     */
    // @formatter:off
    void                            set_output_attachments(std::vector<uint32_t>& output);
    const std::vector<uint32_t>&    get_output_attachments() const;
    
    void            set_layout(uint32_t attachment, vk::ImageLayout layout);
    vk::ImageLayout get_layout(uint32_t attachment) const;
    // @formatter:on

private:
    const vk_device& device;
    vk::Extent2D               extent;
    std::vector<vk_image>      images;
    std::vector<vk_image_view> views;
    std::vector<rt_attachment> attachments;
    std::vector<uint32_t>      input_attachments  = {};         // By default there are no input attachments
    std::vector<uint32_t>      output_attachments = {0};        // By default the output attachments is attachment 0
};