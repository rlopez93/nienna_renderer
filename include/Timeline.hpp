#pragma once
#include "vulkan_raii.hpp"

#include <cstdint>
#include <numeric>
#include <ranges>

#include "Command.hpp"

struct Timeline {

    Timeline(
        Device        &device,
        Queue         &queue,
        const uint32_t maxFramesInFlight);

    auto createTimelineSemaphore(
        Device  &device,
        uint32_t maxFramesInFlight) -> vk::raii::Semaphore;

    auto advance() -> void;

    auto buffer() -> vk::raii::CommandBuffer &;

    auto pool() -> vk::raii::CommandPool &;

    [[nodiscard]]
    auto value() const & -> const uint64_t &;

    auto value() & -> uint64_t &;

    uint32_t              index = 0u;
    std::vector<Command>  command{};
    std::vector<uint64_t> values;
    vk::raii::Semaphore   semaphore;
};
