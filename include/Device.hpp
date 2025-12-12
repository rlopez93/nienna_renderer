#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"

struct Device {
    Device(
        const PhysicalDevice           &physicalDevice,
        const std::vector<std::string> &requiredExtensions);

    static auto createDevice(
        const PhysicalDevice           &physicalDevice,
        const std::vector<std::string> &requiredExtensions) -> vk::raii::Device;

    vk::raii::Device handle;
};
