#include "ResourceAllocator.hpp"
#include <vector>

ResourceAllocator::ResourceAllocator(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount,
    uint32_t          maxFramesInFlight)
    : handle{create(
          device,
          meshCount,
          textureCount,
          maxFramesInFlight)}
{
}

auto ResourceAllocator::create(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount,
    uint32_t          maxFramesInFlight) -> vk::raii::DescriptorPool
{
    auto poolSizes = std::vector<vk::DescriptorPoolSize>{};

    const uint32_t lightCount = 1;

    if (meshCount > 0) {
        poolSizes.emplace_back(
            vk::DescriptorType::eUniformBuffer,
            maxFramesInFlight * (meshCount + lightCount));
    }

    if (textureCount > 0) {
        poolSizes.emplace_back(
            vk::DescriptorType::eCombinedImageSampler,
            maxFramesInFlight * textureCount);
    }

    return {
        device,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            maxFramesInFlight * (textureCount + meshCount + lightCount),
            poolSizes}};
}
