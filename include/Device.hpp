#pragma once

#include <limits>
#include <vulkan/vulkan_raii.hpp>

#include "PhysicalDevice.hpp"
#include "Surface.hpp"

struct QueueFamilyIndices {
    uint32_t graphicsIndex = std::numeric_limits<uint32_t>::max();
    uint32_t presentIndex  = std::numeric_limits<uint32_t>::max();

    auto complete() const -> bool
    {
        return graphicsIndex != std::numeric_limits<uint32_t>::max()
            && presentIndex != std::numeric_limits<uint32_t>::max();
        ;
    }
};

struct Device {
    Device(
        PhysicalDevice const&           physicalDevice,
        Window const&                   window,
        Surface const&                  surface,
        std::vector<char const*> const& requiredExtensions);

    static auto createDevice(
        PhysicalDevice const&           physicalDevice,
        QueueFamilyIndices const&       queueFamilyIndices,
        std::vector<char const*> const& requiredExtensions) -> vk::raii::Device;

    static auto findQueueFamilies(
        PhysicalDevice const& physicalDevice,
        Surface const&        surface) -> QueueFamilyIndices;

    Window const&      window;
    QueueFamilyIndices queueFamilyIndices;
    vk::raii::Device   handle;
    vk::raii::Queue    graphicsQueue;
    vk::raii::Queue    presentQueue;
};
