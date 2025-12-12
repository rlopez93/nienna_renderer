#include "PhysicalDevice.hpp"

#include <algorithm>
#include <fmt/base.h>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

PhysicalDevice::PhysicalDevice(Instance &instance)
    : handle{createPhysicalDevice(instance)}
{
}

// --------------------------------------------------------------
// Helper: return a list of missing extensions for this device
// --------------------------------------------------------------
auto getMissingExtensions(
    const vk::raii::PhysicalDevice &device,
    std::vector<std::string>        requiredExtensions) -> std::vector<std::string>
{
    // Collect device extension names into a vector of string
    auto deviceExtensions =
        device.enumerateDeviceExtensionProperties()
        | std::views::transform([](const auto &extProperties) -> std::string {
              return extProperties.extensionName;
          })
        | std::ranges::to<std::vector>();

    // Find all required extensions that the device does NOT support
    auto missingExts = requiredExtensions
                     | std::views::filter([&](const std::string_view &s) {
                           return !std::ranges::contains(deviceExtensions, s);
                       })
                     | std::ranges::to<std::vector<std::string>>();

    return missingExts;
}

auto createPhysicalDevice(Instance &instance) -> vk::raii::PhysicalDevice
{
    auto physicalDevices = vk::raii::PhysicalDevices{instance.handle};

    if (physicalDevices.empty()) {
        throw std::runtime_error{"No Vulkan physical devices found."};
    }

    const auto requiredExtensions = std::vector{
        std::string{VK_KHR_SWAPCHAIN_EXTENSION_NAME},
        std::string{VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME},
        std::string{VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME},
        std::string{VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME},
        std::string{VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME},
        std::string{VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME},
        std::string{VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME},
        std::string{VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME}};

    std::stringstream diagnostic;

    for (const auto &physicalDevice : physicalDevices) {
        const auto properties = physicalDevice.getProperties();

        if (properties.apiVersion < vk::ApiVersion14) {
            diagnostic << "Device \"" << properties.deviceName
                       << "requires Vulkan 1.4 or newer\n";
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
