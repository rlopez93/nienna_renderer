#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"
#include "Surface.hpp"

struct QueueFamilyIndices {
    uint32_t graphicsIndex = UINT32_MAX;
    uint32_t presentIndex  = UINT32_MAX;

    auto complete() const -> bool
    {
        return graphicsIndex != UINT32_MAX && presentIndex != UINT32_MAX;
    }
};

struct Device {
    Device(
        const PhysicalDevice            &physicalDevice,
        const Window                    &window,
        const Surface                   &surface,
        const std::vector<const char *> &requiredExtensions);

    static auto createDevice(
        const PhysicalDevice            &physicalDevice,
        const QueueFamilyIndices        &queueFamilyIndices,
        const std::vector<const char *> &requiredExtensions) -> vk::raii::Device;

    static auto findQueueFamilies(
        const PhysicalDevice &physicalDevice,
        const Surface        &surface) -> QueueFamilyIndices;

    const Window      &window;
    QueueFamilyIndices queueFamilyIndices;
    vk::raii::Device   handle;
    vk::raii::Queue    graphicsQueue;
    vk::raii::Queue    presentQueue;
};
