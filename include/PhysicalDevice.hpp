#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"

struct PhysicalDevice {

    PhysicalDevice(
        const Instance                 &instance,
        const std::vector<std::string> &requiredExtensions);

    static auto choosePhysicalDevice(
        const Instance                 &instance,
        const std::vector<std::string> &requiredExtensions) -> vk::raii::PhysicalDevice;

    static auto getMissingExtensions(
        const vk::raii::PhysicalDevice &physicalDevice,
        const std::vector<std::string> &requiredExtensions) -> std::vector<std::string>;

    vk::raii::PhysicalDevice handle;
};
