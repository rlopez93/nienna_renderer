#include "Descriptor.hpp"

auto Descriptors::createDescriptorSetLayout(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount,
    uint64_t          maxFramesInFlight) -> vk::raii::DescriptorSetLayout
{
    const auto descriptorSetLayoutBindings = std::array{
        vk::DescriptorSetLayoutBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            meshCount,
            vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{
            1,
            vk::DescriptorType::eCombinedImageSampler,
            textureCount,
            vk::ShaderStageFlagBits::eFragment}};

    return {device, vk::DescriptorSetLayoutCreateInfo{{}, descriptorSetLayoutBindings}};
}

auto Descriptors::createDescriptorPool(
    vk::raii::Device              &device,
    vk::raii::DescriptorSetLayout &descriptorSetLayout,
    uint32_t                       meshCount,
    uint32_t                       textureCount,
    uint64_t                       maxFramesInFlight) -> vk::raii::DescriptorPool
{
    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto poolSizes = std::array{
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(maxFramesInFlight) * meshCount},
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(maxFramesInFlight) * textureCount}};

    return {
        device,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            static_cast<uint32_t>(maxFramesInFlight) * (textureCount + meshCount),
            poolSizes}};
}

auto Descriptors::createDescriptorSets(
    vk::raii::Device              &device,
    vk::raii::DescriptorSetLayout &descriptorSetLayout,
    vk::raii::DescriptorPool      &descriptorPool,
    uint64_t maxFramesInFlight) -> std::vector<vk::raii::DescriptorSet>
{
    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto descriptorSetAllocateInfo =
        vk::DescriptorSetAllocateInfo{descriptorPool, descriptorSetLayouts};

    return device.allocateDescriptorSets(descriptorSetAllocateInfo);
}
