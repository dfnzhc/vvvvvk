/**
 * @File ShaderUtils.hpp
 * @Author dfnzhc (https://github.com/dfnzhc)
 * @Date 2023/11/19
 * @Brief 
 */

#pragma once

#include "ShaderModule.hpp"

#include <spirv_glsl.hpp>
#include <glslang/Public/ShaderLang.h>

class SPIRVReflection
{
public:
    bool reflect_shader_resources(VkShaderStageFlagBits stage,
                                  const std::vector<uint32_t>& spirv,
                                  std::vector<ShaderResource>& resources,
                                  const ShaderVariant& variant);

private:
    void parse_shader_resources(const spirv_cross::Compiler& compiler,
                                VkShaderStageFlagBits stage,
                                std::vector<ShaderResource>& resources,
                                const ShaderVariant& variant);

    void parse_push_constants(const spirv_cross::Compiler& compiler,
                              VkShaderStageFlagBits stage,
                              std::vector<ShaderResource>& resources,
                              const ShaderVariant& variant);

    void parse_specialization_constants(const spirv_cross::Compiler& compiler,
                                        VkShaderStageFlagBits stage,
                                        std::vector<ShaderResource>& resources,
                                        const ShaderVariant& variant);
};

class GLSLCompiler
{
private:
    static glslang::EShTargetLanguage        env_target_language;
    static glslang::EShTargetLanguageVersion env_target_language_version;

public:
    static void set_target_environment(glslang::EShTargetLanguage        target_language,
                                       glslang::EShTargetLanguageVersion target_language_version);

    /**
     * @brief Reset the glslang target environment to the default values
     */
    static void reset_target_environment();

    bool compile_to_spirv(VkShaderStageFlagBits       stage,
                          const std::vector<uint8_t> &glsl_source,
                          const std::string &         entry_point,
                          const ShaderVariant &       shader_variant,
                          std::vector<std::uint32_t> &spirv,
                          std::string &               info_log);
};
