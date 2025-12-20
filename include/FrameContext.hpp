#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

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

    auto timelineSemaphore() const -> vk::Semaphore;

    auto imageAvailableSemaphore() const -> vk::Semaphore;
    auto renderFinishedSemaphore() const -> vk::Semaphore;
    auto currentResourceBindings() const -> vk::DescriptorSet;

    // Timeline synchronization
    vk::raii::Semaphore timelineSemaphoreHandle;

  private:
    static auto createTimelineSemaphore(
        Device  &device,
        uint32_t maxFramesInFlight) -> vk::raii::Semaphore;
    uint32_t frameIndex        = 0;
    uint32_t maxFramesInFlight = 0;

    // Per-frame timeline values
    std::vector<uint64_t> timelineValues;

    // Per-frame command pools + buffers
    std::vector<Command> commands;

    // Per-frame descriptor bindings
    std::vector<vk::raii::DescriptorSet> resourceBindings;

    // Per-frame synchronization
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
};
