#include "Device.hpp"

#include <fmt/base.h>

Device::Device(
    const PhysicalDevice           &physicalDevice,
    const std::vector<std::string> &requiredExtensions)
    : handle{createDevice(
          physicalDevice,
          requiredExtensions)}
{
}

auto Device::createDevice(
    const PhysicalDevice           &physicalDevice,
    const std::vector<std::string> &requiredExtensions) -> vk::raii::Device
{
    return nullptr;
}
