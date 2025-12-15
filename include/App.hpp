#pragma once

#include <glm/ext/vector_float4.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <filesystem>

#include "Allocator.hpp"
#include "Descriptor.hpp"

struct PushConstantBlock {
    uint32_t                transformIndex;
    int32_t                 textureIndex;
    std::array<uint32_t, 2> padding{};
    glm::vec4               baseColorFactor;
};

auto createPipeline(
    vk::raii::Device            &device,
    const std::filesystem::path &shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout    &pipelineLayout) -> vk::raii::Pipeline;

auto updateDescriptorSets(
    vk::raii::Device                       &device,
    Descriptors                            &descriptors,
    const std::vector<Buffer>              &transformUBOs,
    const std::vector<Buffer>              &lightUBOs,
    const uint32_t                         &meshCount,
    const std::vector<vk::raii::ImageView> &textureImageViews,
    vk::raii::Sampler                      &sampler) -> void;
