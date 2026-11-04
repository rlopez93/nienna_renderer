#include "PhysicalDevice.hpp"

#include <fmt/base.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

PhysicalDevice::PhysicalDevice(
    Instance const&                 instance,
    std::vector<char const*> const& requiredExtensions)
    : handle{choosePhysicalDevice(
          instance,
          requiredExtensions)}
{
}

// --------------------------------------------------------------
// Helper: return a list of missing extensions for this device
// --------------------------------------------------------------
auto PhysicalDevice::getMissingExtensions(
    vk::raii::PhysicalDevice const& physicalDevice,
    std::vector<char const*> const& requiredExtensions) -> std::vector<std::string>
{
    // Collect device extension names into a vector<string>
    auto deviceExtensions =
        physicalDevice.enumerateDeviceExtensionProperties()
        | std::views::transform(
            [](vk::ExtensionProperties const& properties) -> std::string {
                return properties.extensionName;
            })
        | std::ranges::to<std::vector>();

    // Find all required extensions that the device does not support
    auto missingExtensions = requiredExtensions
                           | std::views::filter([&](std::string_view s) {
                                 return !std::ranges::contains(deviceExtensions, s);
                             })
                           | std::ranges::to<std::vector<std::string>>();

    return missingExtensions;
}

auto PhysicalDevice::choosePhysicalDevice(
    Instance const&                 instance,
    std::vector<char const*> const& requiredExtensions) -> vk::raii::PhysicalDevice
{
    auto physicalDevices = vk::raii::PhysicalDevices{instance.handle};

    if (physicalDevices.empty()) {
        throw std::runtime_error{"No Vulkan-capable devices found."};
    }

    // collect diagnostic information for if no device is found
    std::stringstream diagnostic;

    for (auto const& physicalDevice : physicalDevices) {
        auto const properties = physicalDevice.getProperties();

        if (properties.apiVersion < vk::ApiVersion14) {
            diagnostic << "Device \"" << properties.deviceName
                       << "\" does not have Vulkan 1.4 or newer\n";
        }

        else if (
            auto missingExtensions =
                getMissingExtensions(physicalDevice, requiredExtensions);
            !missingExtensions.empty()) {
            diagnostic << "Device \"" << properties.deviceName
                       << "\" missing required extensions:\n";
            for (auto const& extension : missingExtensions) {
                diagnostic << "\t" << extension << '\n';
            }
        }

        else if (
            properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu
            || properties.deviceType == vk::PhysicalDeviceType::eIntegratedGpu) {
            return std::move(physicalDevice);
        }
    }

    diagnostic << "No devices satisfied renderer requirements.";

    throw std::runtime_error(diagnostic.str());
}
