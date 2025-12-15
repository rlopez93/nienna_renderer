#include "Frame.hpp"

#include <ranges>

Frame::Frame(
    const Device  &device,
    const uint32_t imagesSize)
{
    recreate(device, imagesSize);
}

auto Frame::recreate(
    const Device  &device,
    const uint32_t imagesSize) -> void
{
    index = 0u;
    imageAvailableSemaphores.clear();
    renderFinishedSemaphores.clear();

    maxFramesInFlight = imagesSize;
    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        imageAvailableSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
    }
}
