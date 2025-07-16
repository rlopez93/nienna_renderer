#include "Descriptor.hpp"

auto Descriptor::createDescriptorSetLayout(
    vk::raii::Device &device,
    uint64_t          maxFramesInFlight) -> vk::raii::DescriptorSetLayout
{
    const auto descriptorSetLayoutBindings = std::array{
        vk::DescriptorSetLayoutBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{
            1,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment}};

    return {device, vk::DescriptorSetLayoutCreateInfo{{}, descriptorSetLayoutBindings}};
}

auto Descriptor::createDescriptorPool(
    vk::raii::Device &device,
    uint64_t          maxFramesInFlight) -> vk::raii::DescriptorPool
{
    auto poolSizes = std::array{
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(maxFramesInFlight)},
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(maxFramesInFlight)}};

    return {
        device,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            static_cast<uint32_t>(maxFramesInFlight),
            poolSizes}};
}

auto Descriptor::createDescriptorSets(
    vk::raii::Device              &device,
    vk::raii::DescriptorPool      &descriptorPool,
    vk::raii::DescriptorSetLayout &descriptorSetLayout,
    uint64_t maxFramesInFlight) -> std::vector<vk::raii::DescriptorSet>
{

    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto descriptorSetAllocateInfo =
        vk::DescriptorSetAllocateInfo{descriptorPool, descriptorSetLayouts};

    return device.allocateDescriptorSets(descriptorSetAllocateInfo);
}
