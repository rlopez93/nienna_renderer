#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Instance.hpp"

struct PhysicalDevice {

    explicit PhysicalDevice(Instance &instance);

    static auto createPhysicalDevice(Instance &instance) -> vk::raii::PhysicalDevice;

    static auto getMissingExtensions(
        const vk::raii::PhysicalDevice &device,
        std::vector<std::string>        requiredExtensions) -> std::vector<std::string>;

    vk::raii::PhysicalDevice handle;
};
