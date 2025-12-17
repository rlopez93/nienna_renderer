#pragma once

#include "Device.hpp"

#include <filesystem>

auto createPipeline(
    Device                      &device,
    const std::filesystem::path &shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout    &pipelineLayout) -> vk::raii::Pipeline;
