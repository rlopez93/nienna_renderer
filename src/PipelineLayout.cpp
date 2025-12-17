// PipelineLayout.cpp
#include "PipelineLayout.hpp"

vk::raii::PipelineLayout createPipelineLayout(
    vk::raii::Device                        &device,
    std::span<const vk::DescriptorSetLayout> setLayouts)
{
    auto pushConstants = std::array{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(PushConstantBlock)}};

    return {device, vk::PipelineLayoutCreateInfo{{}, setLayouts, pushConstants}};
}
