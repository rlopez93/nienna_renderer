#pragma once

#include "vulkan_raii.hpp"

#include "Instance.hpp"
#include "Surface.hpp"

struct PhysicalDevice {

    PhysicalDevice(
        Instance &instance,
        Surface  &surface);

    static auto createPhysicalDevice(
        Instance &instance,
        Surface  &surface) -> vk::raii::PhysicalDevice;

    vk::raii::PhysicalDevice handle;
};
