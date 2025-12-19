#include "FrameContext.hpp"

#include <numeric>

auto FrameContext::createTimelineSemaphore(
    Device  &device,
    uint32_t maxFramesInFlight) -> vk::raii::Semaphore
{
    vk::SemaphoreTypeCreateInfo timelineInfo{
        vk::SemaphoreType::eTimeline,
        maxFramesInFlight - 1};

    return vk::raii::Semaphore{
        device.handle,
        vk::SemaphoreCreateInfo{{}, &timelineInfo}};
}

FrameContext::FrameContext(
    Device  &device,
    uint32_t maxFrames)
    : frameIndex(0),
      maxFramesInFlight(maxFrames),
      timelineSemaphore{createTimelineSemaphore(
          device,
          maxFrames)}
{
    // Per-frame command pools + buffers
    commands.reserve(maxFramesInFlight);
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        commands.emplace_back(
            device,
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
    }

    timelineValues.resize(maxFramesInFlight);
    std::iota(timelineValues.begin(), timelineValues.end(), 0u);
}

auto FrameContext::current() const -> uint32_t
{
    return frameIndex;
}

auto FrameContext::advance() -> void
{
    frameIndex = (frameIndex + 1) % maxFramesInFlight;
}

auto FrameContext::buffer() -> vk::raii::CommandBuffer &
{
    return commands[frameIndex].buffer;
}

auto FrameContext::pool() -> vk::raii::CommandPool &
{
    return commands[frameIndex].pool;
}

auto FrameContext::value() const -> const uint64_t &
{
    return timelineValues[frameIndex];
}

auto FrameContext::value() -> uint64_t &
{
    return timelineValues[frameIndex];
}
