#pragma once
#include <vulkan/vulkan.hpp>

#include <cstdint>
#include <vector>

struct ShaderInterfaceDescription {
    struct Binding {
        uint32_t             binding;
        vk::DescriptorType   type;
        uint32_t             count;
        vk::ShaderStageFlags stages;
    };

    std::vector<Binding> bindings;
};
