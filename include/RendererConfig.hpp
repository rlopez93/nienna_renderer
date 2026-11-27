#pragma once

#include "ShaderInterfaceDescription.hpp"

#include <filesystem>

#include <vulkan/vulkan.hpp>

struct RendererConfig {
    ShaderInterfaceDescription shaderInterfaceDescription;

    std::filesystem::path shaderPath;

    vk::Format colorFormat;
    vk::Format depthFormat;

    uint32_t maxFramesInFlight;
};
