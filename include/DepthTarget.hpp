#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Image.hpp"

struct Device;
struct Allocator;
struct Command;

struct DepthTarget {
    vk::Format          format{};
    Image               image;
    vk::raii::ImageView view = nullptr;

    DepthTarget() = default;

    DepthTarget(
        Device      &device,
        Allocator   &allocator,
        Command     &command,
        vk::Extent2D extent);

    void recreate(
        Device      &device,
        Allocator   &allocator,
        Command     &command,
        vk::Extent2D extent);
};
