#pragma once

#include "vulkan_raii.hpp"

#include "Instance.hpp"
#include "Surface.hpp"

struct PhysicalDevice {

    PhysicalDevice(
        Instance &instance,
        Surface  &surface);

    static auto createVkbPhysicalDevice(
        Instance &instance,
        Surface  &surface) -> vkb::PhysicalDevice;

    vkb::PhysicalDevice      vkbPhysicalDevice;
    vk::raii::PhysicalDevice handle;
};
