#pragma once

#include "vulkan_raii.hpp"

#include "Instance.hpp"
#include "Surface.hpp"

auto createVkbPhysicalDevice(
    Instance &instance,
    Surface  &surface) -> vkb::PhysicalDevice;

struct PhysicalDevice {
    PhysicalDevice(
        Instance &instance,
        Surface  &surface);
    vkb::PhysicalDevice      vkbPhysicalDevice;
    vk::raii::PhysicalDevice handle;
};
