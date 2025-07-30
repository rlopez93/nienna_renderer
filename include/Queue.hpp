#pragma once

#include "vulkan_raii.hpp"

#include "Device.hpp"
enum class QueueType { Graphics, Present, Compute };

auto QueueTypeToVkb(QueueType type) -> vkb::QueueType;

auto createQueue(
    Device   &device,
    QueueType type) -> vk::raii::Queue;

auto getIndex(
    Device   &device,
    QueueType type) -> uint32_t;

struct Queue {
    enum class Type { Graphics, Present, Compute };
    Queue(
        Device   &device,
        QueueType type);
    vk::raii::Queue handle;
    uint32_t        index;
};
