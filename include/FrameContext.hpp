#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Command.hpp"
#include "Device.hpp"

class FrameContext
{
  public:
    FrameContext(
        Device  &device,
        uint32_t maxFramesInFlight);

    // Current frame slot
    [[nodiscard]]
    auto current() const -> uint32_t;
    auto maxFrames() const -> uint32_t;

    // Advance to next frame slot
    auto advance() -> void;

    auto cmd() -> vk::raii::CommandBuffer &;
    auto cmdPool() -> vk::raii::CommandPool &;

    [[nodiscard]]
    auto timelineValue() const -> const uint64_t &;
    auto timelineValue() -> uint64_t &;

    [[nodiscard]]
    auto timelineSemaphore() const -> vk::Semaphore;

    [[nodiscard]]
    auto imageAvailableSemaphore() const -> vk::Semaphore;
    [[nodiscard]]
    auto currentDescriptorSet() const -> vk::DescriptorSet;

    [[nodiscard]]
    auto setDescriptorSets(std::vector<vk::raii::DescriptorSet> &&sets);

    std::vector<Buffer> frameUBO;
    std::vector<Buffer> objectsSSBO;

    // Per-frame descriptor sets
    std::vector<vk::raii::DescriptorSet> descriptorSets;

    auto createPerFrameUniformBuffers(
        Allocator &allocator,
        uint32_t   objectCount) -> void;

  private:
    static auto createTimelineSemaphore(
        Device  &device,
        uint32_t maxFramesInFlight) -> vk::raii::Semaphore;
    uint32_t frameIndex        = 0;
    uint32_t maxFramesInFlight = 0;

    // Timeline synchronization
    vk::raii::Semaphore timelineSemaphoreHandle;

    // Per-frame timeline values
    std::vector<uint64_t> timelineValues;

    // Per-frame command pools + buffers
    std::vector<Command> commands;

    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
};
