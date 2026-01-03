#pragma once

#include <vulkan/vulkan_raii.hpp>

vk::raii::PipelineLayout createPipelineLayout(
    vk::raii::Device                        &device,
    std::span<const vk::DescriptorSetLayout> setLayouts);
