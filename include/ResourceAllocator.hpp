#pragma once
#include <cstdint>
#include <vulkan/vulkan_raii.hpp>

struct ResourceLayout;

struct ResourceAllocator {
    vk::raii::DescriptorPool handle;

    ResourceAllocator(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount,
        uint32_t          maxFramesInFlight);

    static auto create(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount,
        uint32_t          maxFramesInFlight) -> vk::raii::DescriptorPool;
};
