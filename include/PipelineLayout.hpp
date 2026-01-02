#pragma once
// PipelineLayout.hpp
//

#include <vulkan/vulkan_raii.hpp>

#include <glm/ext/vector_float4.hpp>

#include <cstdint>

struct PushConstantBlock {
    uint32_t objectIndex   = 0u;
    uint32_t materialIndex = 0u;
    uint32_t debugView     = 0u;
    uint32_t _pad0         = 0u;
};

vk::raii::PipelineLayout createPipelineLayout(
    vk::raii::Device                        &device,
    std::span<const vk::DescriptorSetLayout> setLayouts);
