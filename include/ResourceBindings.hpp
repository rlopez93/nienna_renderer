#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

struct ResourceLayout;
struct ResourceAllocator;
struct FrameContext;

struct ResourceBindings {
    static auto allocatePerFrame(
        vk::raii::Device     &device,
        const ResourceLayout &layout,
        ResourceAllocator    &allocator,
        FrameContext         &frames) -> void;
};
