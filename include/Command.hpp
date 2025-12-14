#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"

struct Command {
    Command(
        Device                    &device,
        vk::CommandPoolCreateFlags poolFlags = {});

    auto beginSingleTime() -> void;
    auto endSingleTime(Device &device) -> void;

    vk::raii::CommandPool   pool;
    vk::raii::CommandBuffer buffer;
};
