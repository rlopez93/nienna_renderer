#include "Descriptor.hpp"
#include "App.hpp"

Descriptors::Descriptors(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount,
    uint64_t          maxFramesInFlight)
    : descriptorSetLayout{createDescriptorSetLayout(
          device,
          meshCount,
          textureCount,
          maxFramesInFlight)},
      descriptorPool{createDescriptorPool(
          device,
          descriptorSetLayout,
          meshCount,
          textureCount,
          maxFramesInFlight)},
      descriptorSets{createDescriptorSets(
          device,
          descriptorSetLayout,
          descriptorPool,
          maxFramesInFlight)},
      pipelineLayout{createPipelineLayout(
          device,
          descriptorSetLayout,
          maxFramesInFlight)}
{
}

auto Descriptors::createDescriptorSetLayout(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount,
    uint64_t          maxFramesInFlight) -> vk::raii::DescriptorSetLayout
{
    auto descriptorSetLayoutBindings = std::vector<vk::DescriptorSetLayoutBinding>{};

    if (meshCount > 0) {
        descriptorSetLayoutBindings.emplace_back(
            0,
            vk::DescriptorType::eUniformBuffer,
            meshCount,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    }

    if (textureCount > 0) {
        descriptorSetLayoutBindings.emplace_back(
            1,
            vk::DescriptorType::eCombinedImageSampler,
            textureCount,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    };

    descriptorSetLayoutBindings.emplace_back(
        2,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eFragment);

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

    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};

    uint32_t lightCount = 1;

    if (meshCount > 0) {
        poolSizes.emplace_back(
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(maxFramesInFlight) * (meshCount + lightCount));
    }

    if (textureCount > 0) {
        poolSizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(maxFramesInFlight) * textureCount);
    }

    return {
        device,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            static_cast<uint32_t>(maxFramesInFlight)
                * (textureCount + meshCount + lightCount),
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

auto Descriptors::createPipelineLayout(
    vk::raii::Device       &device,
    vk::DescriptorSetLayout descriptorSetLayout,
    uint32_t                maxFramesInFlight) -> vk::raii::PipelineLayout
{
    auto descriptorSetLayouts = std::vector(maxFramesInFlight, descriptorSetLayout);

    auto pushConstantRanges = std::array{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(PushConstantBlock)}};

    return {
        device,
        vk::PipelineLayoutCreateInfo{{}, descriptorSetLayouts, pushConstantRanges}};
}
