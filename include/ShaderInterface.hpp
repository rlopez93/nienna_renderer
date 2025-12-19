#pragma once
#include "Device.hpp"
#include "ShaderInterfaceDescription.hpp"

#include <vulkan/vulkan_raii.hpp>

struct ShaderInterface {
    vk::raii::DescriptorSetLayout handle;

    ShaderInterface(
        Device                           &device,
        const ShaderInterfaceDescription &description);

  private:
    static auto create(
        Device                           &device,
        const ShaderInterfaceDescription &description) -> vk::raii::DescriptorSetLayout;
};
