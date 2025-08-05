#pragma once

#include "vulkan_raii.hpp"

#include "Device.hpp"

struct Queue {
    enum class Type { Graphics, Present, Compute };

    Queue(
        Device &device,
        Type    type);

    static auto QueueTypeToVkb(Type type) -> vkb::QueueType;

    static auto createQueue(
        Device &device,
        Type    type) -> vk::raii::Queue;

    static auto getIndex(
        Device &device,
        Type    type) -> uint32_t;

    vk::raii::Queue handle;
    uint32_t        index;
};
