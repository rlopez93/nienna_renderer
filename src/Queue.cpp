#include "Queue.hpp"

auto QueueTypeToVkb(QueueType type) -> vkb::QueueType
{
    switch (type) {
    case QueueType::Graphics:
        return vkb::QueueType::graphics;

    case QueueType::Present:
        return vkb::QueueType::present;

    case QueueType::Compute:
        return vkb::QueueType::compute;

    default:
        throw std::exception{};
    }
}

auto createQueue(
    Device   &device,
    QueueType type) -> vk::raii::Queue
{
    return {device.handle, device.vkbDevice.get_queue(QueueTypeToVkb(type)).value()};
}

auto getIndex(
    Device   &device,
    QueueType type) -> uint32_t
{
    return device.vkbDevice.get_queue_index(QueueTypeToVkb(type)).value();
}

Queue::Queue(
    Device   &device,
    QueueType type)
    : handle{createQueue(
          device,
          type)},
      index{getIndex(
          device,
          type)}
{
}
