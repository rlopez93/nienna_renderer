#pragma once

#include "Device.hpp"

#include <filesystem>

auto createPipeline(
    Device&                      device,
    std::filesystem::path const& shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout&    pipelineLayout) -> vk::raii::Pipeline;
