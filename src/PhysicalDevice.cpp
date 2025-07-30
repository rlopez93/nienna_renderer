#include "PhysicalDevice.hpp"

#include <algorithm>
#include <fmt/base.h>
#include <iostream>

auto createVkbPhysicalDevice(
    Instance &instance,
    Surface  &surface) -> vkb::PhysicalDevice
{
    // --- Pick physical device and create logical device using vk-bootstrap
    vkb::PhysicalDeviceSelector physicalDeviceSelector{instance.vkbInstance};

    auto features = instance.handle.enumeratePhysicalDevices()
                        .front()
                        .getFeatures2<
                            vk::PhysicalDeviceFeatures2,
                            vk::PhysicalDeviceVulkan11Features,
                            vk::PhysicalDeviceVulkan12Features,
                            vk::PhysicalDeviceVulkan13Features,
                            vk::PhysicalDeviceVulkan14Features,
                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                            vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT,
                            vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT,
                            vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT>();

    auto physicalDeviceResult =
        physicalDeviceSelector.set_surface(*surface.handle)
            .set_required_features(features.get<vk::PhysicalDeviceFeatures2>().features)
            .set_required_features_11(
                features.get<vk::PhysicalDeviceVulkan11Features>())
            .set_required_features_12(
                features.get<vk::PhysicalDeviceVulkan12Features>())
            .set_required_features_13(
                features.get<vk::PhysicalDeviceVulkan13Features>())
            .set_required_features_14(
                features.get<vk::PhysicalDeviceVulkan14Features>())
            .require_present()
            .select();

    if (!physicalDeviceResult) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to select physical device: {}\n",
            physicalDeviceResult.error().message());
        throw std::exception{};
    }

    return physicalDeviceResult.value();
}

PhysicalDevice::PhysicalDevice(
    Instance &instance,
    Surface  &surface)
    : vkbPhysicalDevice{createVkbPhysicalDevice(
          instance,
          surface)},
      handle{
          instance.handle,
          vkbPhysicalDevice.physical_device}
{
}
