#pragma once
#include <vulkan/vulkan_raii.hpp>

struct Descriptor {

    auto createDescriptorSetLayout(
        vk::raii::Device &device,
        uint64_t          maxFramesInFlight,
        bool              hasTexture) -> vk::raii::DescriptorSetLayout;
    auto createDescriptorPool(
        vk::raii::Device &device,
        uint64_t          maxFramesInFlight) -> vk::raii::DescriptorPool;
    auto createDescriptorSets(
        vk::raii::Device              &device,
        vk::raii::DescriptorPool      &descriptorPool,
        vk::raii::DescriptorSetLayout &descriptorSetLayout,
        uint64_t maxFramesInFlight) -> std::vector<vk::raii::DescriptorSet>;
};
