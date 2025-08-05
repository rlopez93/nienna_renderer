#pragma once

#include "vulkan_raii.hpp"

#include "PhysicalDevice.hpp"

struct Device {
    Device(PhysicalDevice &physicalDevice);

    static auto createVkbDevice(PhysicalDevice &physicalDevice) -> vkb::Device;

    vkb::Device      vkbDevice;
    vk::raii::Device handle;
};
