#include "ResourceLayout.hpp"
#include <vector>

ResourceLayout::ResourceLayout(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount)
    : handle{create(
          device,
          meshCount,
          textureCount)}
{
}

auto ResourceLayout::create(
    vk::raii::Device &device,
    uint32_t          meshCount,
    uint32_t          textureCount) -> vk::raii::DescriptorSetLayout
{
    auto bindings = std::vector<vk::DescriptorSetLayoutBinding>{};

    if (meshCount > 0) {
        bindings.emplace_back(
            0,
            vk::DescriptorType::eUniformBuffer,
            meshCount,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    }

    if (textureCount > 0) {
        bindings.emplace_back(
            1,
            vk::DescriptorType::eCombinedImageSampler,
            textureCount,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
    }

    // Light UBO (always 1 in your code)
    bindings.emplace_back(
        2,
        vk::DescriptorType::eUniformBuffer,
        1,
        vk::ShaderStageFlagBits::eFragment);

    return {device, vk::DescriptorSetLayoutCreateInfo{{}, bindings}};
}
