#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <spirv_reflect.h>

#include <filesystem>
#include <vector>

struct VertexReflectionData {

    std::vector<SpvReflectInterfaceVariable*> variables;
    std::vector<SpvReflectDescriptorBinding*> bindings;
    std::vector<SpvReflectDescriptorSet*>     sets;
};

auto readShaders(std::string const& filename) -> std::vector<char>;
auto reflectShader(std::vector<char> const& code) -> VertexReflectionData;
auto createShaderModule(
    vk::raii::Device&            device,
    std::filesystem::path const& path) -> vk::raii::ShaderModule;
