#include "ShaderInterface.hpp"
#include <vector>

#include "ShaderInterface.hpp"

#include <vector>

ShaderInterface::ShaderInterface(
    Device                           &device,
    const ShaderInterfaceDescription &description)
    : handle{create(
          device,
          description)}
{
}

auto ShaderInterface::create(
    Device                           &device,
    const ShaderInterfaceDescription &description) -> vk::raii::DescriptorSetLayout
{
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(description.bindings.size());

    for (const auto &b : description.bindings) {
        bindings.emplace_back(b.binding, b.type, b.count, b.stages);
    }

    return {device.handle, vk::DescriptorSetLayoutCreateInfo{{}, bindings}};
}

// ShaderInterface::ShaderInterface(
//     vk::raii::Device &device,
//     uint32_t          meshCount,
//     uint32_t          textureCount)
//     : handle{create(
//           device,
//           meshCount,
//           textureCount)}
// {
// }
//
// auto ShaderInterface::create(
//     vk::raii::Device &device,
//     uint32_t          meshCount,
//     uint32_t          textureCount) -> vk::raii::DescriptorSetLayout
// {
//     auto bindings = std::vector<vk::DescriptorSetLayoutBinding>{};
//
//     if (meshCount > 0) {
//         bindings.emplace_back(
//             0,
//             vk::DescriptorType::eUniformBuffer,
//             meshCount,
//             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
//     }
//
//     if (textureCount > 0) {
//         bindings.emplace_back(
//             1,
//             vk::DescriptorType::eCombinedImageSampler,
//             textureCount,
//             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment);
//     }
//
//     // Light UBO (always 1 in your code)
//     bindings.emplace_back(
//         2,
//         vk::DescriptorType::eUniformBuffer,
//         1,
//         vk::ShaderStageFlagBits::eFragment);
//
//     return {device, vk::DescriptorSetLayoutCreateInfo{{}, bindings}};
// }
