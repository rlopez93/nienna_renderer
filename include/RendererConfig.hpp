#pragma once

#include "ShaderInterfaceDescription.hpp"

#include <filesystem>

#include <vulkan/vulkan.hpp>

struct RendererConfig {
    ShaderInterfaceDescription shaderInterfaceDescription;

    // Shader modules / program
    std::filesystem::path shaderPath;

    // Render target formats (swapchain + depth)
    vk::Format colorFormat;
    vk::Format depthFormat;
};
