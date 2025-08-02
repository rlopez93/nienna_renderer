#pragma once

#include <cstdint>
#include <vector>

#include "vulkan_raii.hpp"

#include "Device.hpp"

struct Frame {
    uint32_t                         index = 0u;
    uint32_t                         maxFramesInFlight;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    Frame(
        Device  &device,
        uint32_t imagesSize);

    auto recreate(
        Device  &device,
        uint32_t imagesSize) -> void;
};
