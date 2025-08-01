#include "Device.hpp"

#include <fmt/base.h>

auto createVkbDevice(PhysicalDevice &physicalDevice) -> vkb::Device
{
    auto deviceBuilder = vkb::DeviceBuilder{physicalDevice.vkbPhysicalDevice};
    auto deviceResult  = deviceBuilder.build();
    if (!deviceResult) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create logical device: {}\n",
            deviceResult.error().message());
        throw std::exception{};
    }

    return deviceResult.value();
}
Device::Device(PhysicalDevice &physicalDevice)
    : vkbDevice{createVkbDevice(physicalDevice)},
      handle{
          physicalDevice.handle,
          vkbDevice.device}
{
}
