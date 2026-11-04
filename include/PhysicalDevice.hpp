#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"

struct PhysicalDevice {

    PhysicalDevice(
        Instance const&                 instance,
        std::vector<char const*> const& requiredExtensions);

    static auto choosePhysicalDevice(
        Instance const&                 instance,
        std::vector<char const*> const& requiredExtensions) -> vk::raii::PhysicalDevice;

    static auto getMissingExtensions(
        vk::raii::PhysicalDevice const& physicalDevice,
        std::vector<char const*> const& requiredExtensions) -> std::vector<std::string>;

    vk::raii::PhysicalDevice handle;
};
