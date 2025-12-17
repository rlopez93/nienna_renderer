#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

struct FrameContext {
    explicit FrameContext(uint32_t framesInFlight)
        : commandBuffers(framesInFlight),
          descriptorSets(framesInFlight),
          uniformBuffers(framesInFlight),
          frameTimelineValue(
              framesInFlight,
              0),
          swapchainImageIndex(
              framesInFlight,
              0)
    {
    }
    std::vector<vk::CommandBuffer> commandBuffers;
    std::vector<vk::DescriptorSet> descriptorSets;
    std::vector<vk::Buffer>        uniformBuffers;
    std::vector<uint64_t>          frameTimelineValue;
    std::vector<uint32_t>          swapchainImageIndex;
};
