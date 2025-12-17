#pragma once
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

struct ResourceLayout {
    vk::raii::DescriptorSetLayout handle;

    ResourceLayout(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount);

    static auto create(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount) -> vk::raii::DescriptorSetLayout;
};
