#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Image.hpp"

struct Device;
struct Allocator;
struct Command;

struct DepthTarget {
    vk::Format          format{};
    Image               image;
    vk::raii::ImageView view;

    void recreate(
        Device      &device,
        Allocator   &allocator,
        Command     &command,
        vk::Extent2D extent);
};
