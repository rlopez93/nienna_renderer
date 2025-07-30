#include "PhysicalDevice.hpp"

#include <fmt/base.h>

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
            .require_present()
            .select();

    if (!physicalDeviceResult) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to select physical device: {}\n",
            physicalDeviceResult.error().message());
        throw std::exception{};
    }

    auto vkbPhysicalDevice = physicalDeviceResult.value();
    // auto deviceExtensions = vk::getDeviceExtensions();
    //
    // fmt::print(stderr, "device extensions: {}\n", deviceExtensions.size());
    //
    // for (const auto &extension : deviceExtensions) {
    //     std::vector<const char *> enabledExtensions;
    //     enabledExtensions.push_back(extension.c_str());
    //     bool enabled = vkb_phys.enable_extensions_if_present(enabledExtensions);
    //
    //     fmt::print(stderr, "<{}> enabled: {}\n", extension, enabled);
    // }

    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceFeatures2>());
    vkbPhysicalDevice.enable_extension_features_if_present(
        features.get<vk::PhysicalDeviceVulkan11Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan12Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan13Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan14Features>());

    return vkbPhysicalDevice;
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
