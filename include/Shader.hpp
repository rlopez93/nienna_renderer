#pragma once

#include "vulkan_raii.hpp"

#include <spirv_reflect.h>

#include <filesystem>
#include <vector>

struct VertexReflectionData {

    std::vector<SpvReflectInterfaceVariable *> variables;
    std::vector<SpvReflectDescriptorBinding *> bindings;
    std::vector<SpvReflectDescriptorSet *>     sets;
};

auto readShaders(const std::string &filename) -> std::vector<char>;
auto reflectShader(const std::vector<char> &code) -> VertexReflectionData;
auto createShaderModule(
    vk::raii::Device            &device,
    const std::filesystem::path &path) -> vk::raii::ShaderModule;
