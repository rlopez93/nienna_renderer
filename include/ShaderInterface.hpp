#pragma once
#include "Device.hpp"
#include "ShaderInterfaceDescription.hpp"

#include <vulkan/vulkan_raii.hpp>

struct ShaderInterface {
    vk::raii::DescriptorSetLayout handle;

    ShaderInterface(
        Device&                           device,
        ShaderInterfaceDescription const& description);

  private:
    static auto create(
        Device&                           device,
        ShaderInterfaceDescription const& description) -> vk::raii::DescriptorSetLayout;
};
