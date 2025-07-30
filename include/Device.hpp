#pragma once

#include "vulkan_raii.hpp"

#include "PhysicalDevice.hpp"

auto createVkbDevice(PhysicalDevice &physicalDevice) -> vkb::Device;

struct Device {
    Device(PhysicalDevice &physicalDevice);

    vkb::Device      vkbDevice;
    vk::raii::Device handle;
};
