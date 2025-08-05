#pragma once

#include "vulkan_raii.hpp"

#include "Device.hpp"
#include "Queue.hpp"

struct Command {
    Command(
        Device                    &device,
        Queue                     &queue,
        vk::CommandPoolCreateFlags poolFlags = {});

    auto beginSingleTime() -> void;
    auto endSingleTime(
        Device &device,
        Queue  &queue) -> void;

    vk::raii::CommandPool   pool;
    vk::raii::CommandBuffer buffer;
};
