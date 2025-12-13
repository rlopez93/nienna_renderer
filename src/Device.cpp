#include "Device.hpp"

#include <fmt/base.h>

using FeatureChain = vk::StructureChain<
    vk::PhysicalDeviceFeatures2,
    vk::PhysicalDeviceVulkan11Features,
    vk::PhysicalDeviceVulkan12Features,
    vk::PhysicalDeviceVulkan13Features,
    vk::PhysicalDeviceVulkan14Features>;

Device::Device(
    const PhysicalDevice           &physicalDevice,
    const Surface                  &surface,
    const std::vector<std::string> &requiredExtensions)
    : queueFamilyIndices{findQueueFamilies(
          physicalDevice,
          surface)},
      handle{createDevice(
          physicalDevice,
          queueFamilyIndices,
          requiredExtensions)},
      graphicsQueue(
          handle,
          queueFamilyIndices.graphicsIndex,
          0),
      presentQueue(
          handle,
          queueFamilyIndices.presentIndex,
          0)
{
}

auto Device::createDevice(
    const PhysicalDevice           &physicalDevice,
    const QueueFamilyIndices       &queueFamilyIndices,
    const std::vector<std::string> &requiredExtensions) -> vk::raii::Device
{
    return nullptr;
}

auto Device::findQueueFamilies(
    const PhysicalDevice &physicalDevice,
    const Surface        &surface) -> QueueFamilyIndices
{
    auto queueFamilyIndices = QueueFamilyIndices{};

    const auto queueFamilyProperties = physicalDevice.handle.getQueueFamilyProperties();

    for (uint32_t index = 0; index < queueFamilyProperties.size(); ++index) {
        const auto &properties = queueFamilyProperties[index];

        if ((properties.queueFlags & vk::QueueFlagBits::eGraphics)
            && queueFamilyIndices.graphicsIndex == UINT32_MAX) {
            queueFamilyIndices.graphicsIndex = index;
        }

        if (physicalDevice.handle.getSurfaceSupportKHR(index, *surface.handle)
            && queueFamilyIndices.presentIndex == UINT32_MAX) {
            queueFamilyIndices.presentIndex = index;
        }

        if (queueFamilyIndices.graphicsIndex != UINT32_MAX
            && queueFamilyIndices.presentIndex != UINT32_MAX) {
            break;
        }
    }

    if (queueFamilyIndices.graphicsIndex == UINT32_MAX
        && queueFamilyIndices.presentIndex == UINT32_MAX) {
        throw std::runtime_error(
            "Device does not have necessary graphics queue and present queue.");
    }

    else if (queueFamilyIndices.graphicsIndex != queueFamilyIndices.presentIndex) {
        throw std::runtime_error(
            "Device does not have unified graphics + present queue. Support for "
            "separate queues not implemented yet.");
    }

    return queueFamilyIndices;
}
