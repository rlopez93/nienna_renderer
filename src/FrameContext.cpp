#include "FrameContext.hpp"
#include "Scene.hpp"

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
      timelineSemaphoreHandle{createTimelineSemaphore(
          device,
          maxFrames)}
{
    if (maxFrames == 0) {
        throw std::invalid_argument("maxFramesInFlight must be >= 1");
    }

    commands.reserve(maxFramesInFlight);
    imageAvailableSemaphores.reserve(maxFramesInFlight);
    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        commands.emplace_back(
            device,
            vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
        // Binary semaphores per frame
        imageAvailableSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
    }

    timelineValues.resize(maxFramesInFlight);
    std::iota(timelineValues.begin(), timelineValues.end(), 0u);
}

auto FrameContext::current() const -> uint32_t
{
    return frameIndex;
}

auto FrameContext::maxFrames() const -> uint32_t
{
    return maxFramesInFlight;
}
auto FrameContext::advance() -> void
{
    frameIndex = (frameIndex + 1) % maxFramesInFlight;
}

auto FrameContext::cmd() -> vk::raii::CommandBuffer &
{
    return commands[frameIndex].buffer;
}

auto FrameContext::cmdPool() -> vk::raii::CommandPool &
{
    return commands[frameIndex].pool;
}

auto FrameContext::timelineValue() const -> const uint64_t &
{
    return timelineValues[frameIndex];
}

auto FrameContext::timelineValue() -> uint64_t &
{
    return timelineValues[frameIndex];
}

auto FrameContext::timelineSemaphore() const -> vk::Semaphore
{
    return *timelineSemaphoreHandle;
}

auto FrameContext::imageAvailableSemaphore() const -> vk::Semaphore
{
    return *imageAvailableSemaphores[frameIndex];
}

auto FrameContext::currentDescriptorSet() const -> vk::DescriptorSet
{
    return *descriptorSets[frameIndex];
}

auto FrameContext::setDescriptorSets(std::vector<vk::raii::DescriptorSet> &&sets)
{
    descriptorSets = std::move(sets);
}

auto FrameContext::createPerFrameUniformBuffers(
    Allocator &allocator,
    uint32_t   meshCount) -> void
{
    // Recreate-safe: clear and rebuild
    transformUBO.clear();
    lightUBO.clear();

    transformUBO.reserve(maxFramesInFlight);
    lightUBO.reserve(maxFramesInFlight);

    const vk::DeviceSize transformBytes = static_cast<vk::DeviceSize>(sizeof(Transform))
                                        * static_cast<vk::DeviceSize>(meshCount);

    for (uint32_t i = 0; i < maxFramesInFlight; ++i) {
        transformUBO.emplace_back(allocator.createBuffer(
            transformBytes,
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            false,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));

        lightUBO.emplace_back(allocator.createBuffer(
            sizeof(Light),
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            false,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    }
}
