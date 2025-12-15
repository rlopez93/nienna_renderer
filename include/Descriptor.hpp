#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

struct Descriptors {
    Descriptors(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount,
        uint64_t          maxFramesInFlight);

    vk::raii::DescriptorSetLayout        descriptorSetLayout;
    vk::raii::DescriptorPool             descriptorPool;
    std::vector<vk::raii::DescriptorSet> descriptorSets;
    vk::raii::PipelineLayout             pipelineLayout;

  private:
    static auto createDescriptorSetLayout(
        vk::raii::Device &device,
        uint32_t          meshCount,
        uint32_t          textureCount,
        uint64_t          maxFramesInFlight) -> vk::raii::DescriptorSetLayout;

    static auto createDescriptorPool(
        vk::raii::Device              &device,
        vk::raii::DescriptorSetLayout &descriptorSetLayout,
        uint32_t                       meshCount,
        uint32_t                       textureCount,
        uint64_t                       maxFramesInFlight) -> vk::raii::DescriptorPool;

    static auto createDescriptorSets(
        vk::raii::Device              &device,
        vk::raii::DescriptorSetLayout &descriptorSetLayout,
        vk::raii::DescriptorPool      &descriptorPool,
        uint64_t maxFramesInFlight) -> std::vector<vk::raii::DescriptorSet>;

    static auto createPipelineLayout(
        vk::raii::Device       &device,
        vk::DescriptorSetLayout descriptorSetLayout,
        uint32_t                maxFramesInFlight) -> vk::raii::PipelineLayout;
};
