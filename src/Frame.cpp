#include "Frame.hpp"

#include <ranges>

Frame::Frame(
    Device  &device,
    uint32_t imagesSize)
{
    recreate(device, imagesSize);
}

auto Frame::recreate(
    Device  &device,
    uint32_t imagesSize) -> void
{
    currentFrameIndex = 0u;
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();

    maxFramesInFlight = imagesSize;
    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        imageAvailableSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
    }
}
