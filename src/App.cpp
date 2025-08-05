#include "App.hpp"
#include "Shader.hpp"

#include <ranges>

auto createPipeline(
    vk::raii::Device            &device,
    const std::filesystem::path &shaderPath,
    vk::Format                   imageFormat,
    vk::Format                   depthFormat,
    vk::raii::PipelineLayout    &pipelineLayout) -> vk::raii::Pipeline
{
    auto shaderModule = createShaderModule(device, shaderPath);

    // The stages used by this pipeline
    const auto shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            shaderModule,
            "vertexMain",
            {}},
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            shaderModule,
            "fragmentMain",
            {}},
    };

    const auto vertexBindingDescriptions = std::array{vk::VertexInputBindingDescription{
        0,
        sizeof(Primitive),
        vk::VertexInputRate::eVertex}};

    const auto vertexAttributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Primitive, position)},

        vk::VertexInputAttributeDescription{
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Primitive, normal)},
        vk::VertexInputAttributeDescription{
            2,
            0,
            vk::Format::eR32G32Sfloat,
            offsetof(Primitive, uv)},
        vk::VertexInputAttributeDescription{
            3,
            0,
            vk::Format::eR32G32B32A32Sfloat,
            offsetof(Primitive, color)},
    };

    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        {},
        vertexBindingDescriptions,
        vertexAttributeDescriptions};

    const auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        false};

    const auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{};

    const auto pipelineRasterizationStateCreateInfo =
        vk::PipelineRasterizationStateCreateInfo{
            {},
            {},
            {},
            vk::PolygonMode::eFill,
            vk::CullModeFlags::BitsType::eBack,
            vk::FrontFace::eCounterClockwise,
            {},
            {},
            {},
            {},
            1.0f};

    const auto pipelineMultisampleStateCreateInfo =
        vk::PipelineMultisampleStateCreateInfo{};

    const auto pipelineDepthStencilStateCreateInfo =
        vk::PipelineDepthStencilStateCreateInfo{
            {},
            true,
            true,
            vk::CompareOp::eLessOrEqual};

    const auto colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    const auto pipelineColorBlendStateCreateInfo =
        vk::PipelineColorBlendStateCreateInfo{
            {},
            {},
            vk::LogicOp::eCopy,
            1,
            &colorBlendAttachmentState};

    const auto dynamicStates = std::array{
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount};

    const auto pipelineDynamicStateCreateInfo =
        vk::PipelineDynamicStateCreateInfo{{}, dynamicStates};

    auto pipelineRenderingCreateInfo =
        vk::PipelineRenderingCreateInfo{{}, imageFormat, depthFormat};

    auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{
        {},
        shaderStages,
        &vertexInputStateCreateInfo,
        &inputAssemblyCreateInfo,
        {},
        &pipelineViewportStateCreateInfo,
        &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo,
        &pipelineDepthStencilStateCreateInfo,
        &pipelineColorBlendStateCreateInfo,
        &pipelineDynamicStateCreateInfo,
        *pipelineLayout,
        {},
        {},
        {},
        {},
        &pipelineRenderingCreateInfo};

    return vk::raii::Pipeline{device, nullptr, graphicsPipelineCreateInfo};
}

void updateDescriptorSets(
    vk::raii::Device                       &device,
    Descriptors                            &descriptors,
    const std::vector<Buffer>              &sceneBuffers,
    const uint32_t                         &meshCount,
    const std::vector<vk::raii::ImageView> &textureImageViews,
    vk::raii::Sampler                      &sampler)
{
    for (auto frameIndex : std::views::iota(0u, sceneBuffers.size())) {
        auto descriptorWrites = std::vector<vk::WriteDescriptorSet>{};

        auto descriptorBufferInfos = std::vector<vk::DescriptorBufferInfo>{};

        vk::DeviceSize transformBufferSize = sizeof(Transform) * meshCount;
        for (auto meshIndex : std::views::iota(0, static_cast<int32_t>(meshCount))) {
            descriptorBufferInfos.emplace_back(
                sceneBuffers[frameIndex].buffer,
                sizeof(Transform) * meshIndex,
                sizeof(Transform));
        }

        if (!descriptorBufferInfos.empty()) {
            descriptorWrites.emplace_back(
                vk::WriteDescriptorSet{
                    descriptors.descriptorSets[frameIndex],
                    0,
                    0,
                    vk::DescriptorType::eUniformBuffer,
                    {},
                    descriptorBufferInfos});
        }

        auto descriptorImageInfos = std::vector<vk::DescriptorImageInfo>{};
        for (const auto &view : textureImageViews) {
            descriptorImageInfos.emplace_back(
                sampler,
                *view,
                vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        if (!descriptorImageInfos.empty()) {
            descriptorWrites.emplace_back(
                descriptors.descriptorSets[frameIndex],
                1,
                0,
                vk::DescriptorType::eCombinedImageSampler,
                descriptorImageInfos);
        }

        device.updateDescriptorSets(descriptorWrites, {});
    }
}
