#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct FrameContext {
    uint32_t frameIndex        = 0;
    uint32_t maxFramesInFlight = 0;

    // Per-frame descriptor bindings
    std::vector<vk::raii::DescriptorSet> resourceBindings;

    auto current() const -> uint32_t
    {
        return frameIndex;
    }

    auto advance() -> void
    {
        frameIndex = (frameIndex + 1) % maxFramesInFlight;
    }
};
