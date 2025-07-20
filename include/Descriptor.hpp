#pragma once
#include <vulkan/vulkan_raii.hpp>

struct Descriptors {
    Descriptors(
        vk::raii::Device &device,
        uint32_t          textureCount,
        uint64_t          maxFramesInFlight)
        : descriptorSetLayout{createDescriptorSetLayout(
              device,
              textureCount,
              maxFramesInFlight)},
          descriptorPool{createDescriptorPool(
              device,
              descriptorSetLayout,
              textureCount,
              maxFramesInFlight)},
          descriptorSets{createDescriptorSets(
              device,
              descriptorSetLayout,
              descriptorPool,
              textureCount,
              maxFramesInFlight)}
    {
    }

    vk::raii::DescriptorSetLayout        descriptorSetLayout;
    vk::raii::DescriptorPool             descriptorPool;
    std::vector<vk::raii::DescriptorSet> descriptorSets;

  private:
    static auto createDescriptorSetLayout(
        vk::raii::Device &device,
        uint32_t          textureCount,
        uint64_t          maxFramesInFlight) -> vk::raii::DescriptorSetLayout;

    static auto createDescriptorPool(
        vk::raii::Device              &device,
        vk::raii::DescriptorSetLayout &descriptorSetLayout,
        uint32_t                       textureCount,
        uint64_t                       maxFramesInFlight) -> vk::raii::DescriptorPool;

    static auto createDescriptorSets(
        vk::raii::Device              &device,
        vk::raii::DescriptorSetLayout &descriptorSetLayout,
        vk::raii::DescriptorPool      &descriptorPool,
        uint32_t                       textureCount,
        uint64_t maxFramesInFlight) -> std::vector<vk::raii::DescriptorSet>;
};
