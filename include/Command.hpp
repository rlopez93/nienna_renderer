#pragma once

#include "vulkan_raii.hpp"

#include "Queue.hpp"

struct Command {
    Command(
        Device                    &device,
        Queue                     &queue,
        vk::CommandPoolCreateFlags poolFlags = {});

    vk::raii::CommandPool   pool;
    vk::raii::CommandBuffer buffer;
};
