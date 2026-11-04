#include "Device.hpp"

#include <fmt/base.h>
#include <set>
#include <vector>

Device::Device(
    PhysicalDevice const&           physicalDevice,
    Window const&                   window,
    Surface const&                  surface,
    std::vector<char const*> const& requiredExtensions)
    : window{window},
      queueFamilyIndices{findQueueFamilies(
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
    PhysicalDevice const&           physicalDevice,
    QueueFamilyIndices const&       queueFamilyIndices,
    std::vector<char const*> const& requiredExtensions) -> vk::raii::Device
{

    auto features2 = physicalDevice.handle.getFeatures2<
        // Core container
        vk::PhysicalDeviceFeatures2,

        // Core Vulkan versions
        vk::PhysicalDeviceVulkan11Features,
        vk::PhysicalDeviceVulkan12Features,
        vk::PhysicalDeviceVulkan13Features,
        vk::PhysicalDeviceVulkan14Features,

        // Descriptor model
        vk::PhysicalDeviceDescriptorBufferFeaturesEXT,

        // Dynamic state
        vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT,
        vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT>();

    auto const queuePriority = 1.0f;

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t>                     uniqueQueueFamilies = {
        queueFamilyIndices.graphicsIndex,
        queueFamilyIndices.presentIndex};

    for (uint32_t familyIndex : uniqueQueueFamilies) {
        queueCreateInfos
            .emplace_back(vk::DeviceQueueCreateFlags{}, familyIndex, 1, &queuePriority);
    }

    auto deviceCreateInfo = vk::DeviceCreateInfo{
        {},
        queueCreateInfos,
        {},
        requiredExtensions,
        {},
        &features2.get<vk::PhysicalDeviceFeatures2>()};

    return {physicalDevice.handle, deviceCreateInfo};
}

auto Device::findQueueFamilies(
    PhysicalDevice const& physicalDevice,
    Surface const&        surface) -> QueueFamilyIndices
{
    auto queueFamilyIndices = QueueFamilyIndices{};

    auto const queueFamilyProperties = physicalDevice.handle.getQueueFamilyProperties();

    for (uint32_t index = 0; index < queueFamilyProperties.size(); ++index) {
        auto const& properties = queueFamilyProperties[index];

        if ((properties.queueFlags & vk::QueueFlagBits::eGraphics)
            && queueFamilyIndices.graphicsIndex
                   == std::numeric_limits<uint32_t>::max()) {
            queueFamilyIndices.graphicsIndex = index;
        }

        if (physicalDevice.handle.getSurfaceSupportKHR(index, *surface.handle)
            && queueFamilyIndices.presentIndex
                   == std::numeric_limits<uint32_t>::max()) {
            queueFamilyIndices.presentIndex = index;
        }

        if (queueFamilyIndices.graphicsIndex != std::numeric_limits<uint32_t>::max()
            && queueFamilyIndices.presentIndex
                   != std::numeric_limits<uint32_t>::max()) {
            break;
        }
    }

    if (queueFamilyIndices.graphicsIndex == std::numeric_limits<uint32_t>::max()
        && queueFamilyIndices.presentIndex == std::numeric_limits<uint32_t>::max()) {
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
