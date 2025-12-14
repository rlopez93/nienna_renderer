#include "PhysicalDevice.hpp"

#include <fmt/base.h>

#include <algorithm>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

PhysicalDevice::PhysicalDevice(
    const Instance                  &instance,
    const std::vector<const char *> &requiredExtensions)
    : handle{choosePhysicalDevice(
          instance,
          requiredExtensions)}
{
}

// --------------------------------------------------------------
// Helper: return a list of missing extensions for this device
// --------------------------------------------------------------
auto PhysicalDevice::getMissingExtensions(
    const vk::raii::PhysicalDevice  &physicalDevice,
    const std::vector<const char *> &requiredExtensions) -> std::vector<std::string>
{
    // Collect device extension names into a vector<string>
    auto deviceExtensions =
        physicalDevice.enumerateDeviceExtensionProperties()
        | std::views::transform(
            [](const vk::ExtensionProperties &properties) -> std::string {
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
    const Instance                  &instance,
    const std::vector<const char *> &requiredExtensions) -> vk::raii::PhysicalDevice
{
    auto physicalDevices = vk::raii::PhysicalDevices{instance.handle};

    if (physicalDevices.empty()) {
        throw std::runtime_error{"No Vulkan-capable devices found."};
    }

    // collect diagnostic information for if no device is found
    std::stringstream diagnostic;

    for (const auto &physicalDevice : physicalDevices) {
        const auto properties = physicalDevice.getProperties();

        if (properties.apiVersion < vk::ApiVersion14) {
            diagnostic << "Device \"" << properties.deviceName
                       << "\" does not have Vulkan 1.4 or newer\n";
        }

        else if (auto missingExtensions =
                     getMissingExtensions(physicalDevice, requiredExtensions);
                 !missingExtensions.empty()) {
            diagnostic << "Device \"" << properties.deviceName
                       << "\" missing required extensions:\n";
            for (const auto &extension : missingExtensions) {
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
