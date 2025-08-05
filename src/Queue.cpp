#include "Queue.hpp"

auto Queue::QueueTypeToVkb(Type type) -> vkb::QueueType
{
    switch (type) {
    case Type::Graphics:
        return vkb::QueueType::graphics;

    case Type::Present:
        return vkb::QueueType::present;

    case Type::Compute:
        return vkb::QueueType::compute;

    default:
        throw std::exception{};
    }
}

auto Queue::createQueue(
    Device &device,
    Type    type) -> vk::raii::Queue
{
    return {device.handle, device.vkbDevice.get_queue(QueueTypeToVkb(type)).value()};
}

auto Queue::getIndex(
    Device &device,
    Type    type) -> uint32_t
{
    return device.vkbDevice.get_queue_index(QueueTypeToVkb(type)).value();
}

Queue::Queue(
    Device &device,
    Type    type)
    : handle{createQueue(
          device,
          type)},
      index{getIndex(
          device,
          type)}
{
}
