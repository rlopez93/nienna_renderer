#include "Timeline.hpp"

auto Timeline::createTimelineSemaphore(
    Device  &device,
    uint32_t maxFramesInFlight) -> vk::raii::Semaphore
{
    auto timelineSemaphoreCreateInfo = vk::SemaphoreTypeCreateInfo{
        vk::SemaphoreType::eTimeline,
        maxFramesInFlight - 1};

    return {device.handle, vk::SemaphoreCreateInfo{{}, &timelineSemaphoreCreateInfo}};
}

Timeline::Timeline(
    Device        &device,
    Queue         &queue,
    const uint32_t maxFramesInFlight)
    : values{
          maxFramesInFlight,
          0}
    , semaphore{createTimelineSemaphore(device, maxFramesInFlight)}

{
    for (int n : std::views::iota(0u, maxFramesInFlight)) {
        command.emplace_back(device, queue);
    }

    std::ranges::iota(values, 0u);
}
auto Timeline::advance() -> void
{
    index = (index + 1) % values.size();
}
auto Timeline::buffer() -> vk::raii::CommandBuffer &
{
    return command[index].buffer;
}
auto Timeline::value() const & -> const uint64_t &
{
    return values[index];
}
auto Timeline::value() & -> uint64_t &
{
    return values[index];
}
auto Timeline::pool() -> vk::raii::CommandPool &
{
    return command[index].pool;
}
