#pragma once
// PipelineLayout.hpp
//

#include <vulkan/vulkan_raii.hpp>

#include <glm/ext/vector_float4.hpp>

#include <array>
#include <cstdint>

struct PushConstantBlock {
    uint32_t                transformIndex;
    int32_t                 textureIndex;
    std::array<uint32_t, 2> padding{};
    glm::vec4               baseColorFactor;
};

vk::raii::PipelineLayout createPipelineLayout(
    vk::raii::Device                        &device,
    std::span<const vk::DescriptorSetLayout> setLayouts);
