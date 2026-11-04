#pragma once

#include <vulkan/vulkan_raii.hpp>

auto createPipelineLayout(
    vk::raii::Device&                        device,
    std::span<vk::DescriptorSetLayout const> setLayouts) -> vk::raii::PipelineLayout;
