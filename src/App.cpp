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
    auto       shaderModule = createShaderModule(device, shaderPath);
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

auto createBuffers(
    vk::raii::Device      &device,
    vk::raii::CommandPool &cmdPool,
    Allocator             &allocator,
    vk::raii::Queue       &queue,
    const Scene           &scene,
    uint64_t               maxFramesInFlight)
    -> std::tuple<
        std::vector<Buffer>,
        std::vector<Buffer>,
        std::vector<Buffer>,
        std::vector<Image>,
        std::vector<vk::raii::ImageView>>
{
    auto primitiveBuffers    = std::vector<Buffer>{};
    auto indexBuffers        = std::vector<Buffer>{};
    auto sceneBuffers        = std::vector<Buffer>{};
    auto textureImageBuffers = std::vector<Image>{};
    auto textureImageViews   = std::vector<vk::raii::ImageView>{};

    auto cmd = beginSingleTimeCommands(device, cmdPool);

    for (const auto &mesh : scene.meshes) {
        primitiveBuffers.emplace_back(allocator.createBufferAndUploadData(
            cmd,
            mesh.primitives,
            vk::BufferUsageFlagBits2::eVertexBuffer));

        indexBuffers.emplace_back(allocator.createBufferAndUploadData(
            cmd,
            mesh.indices,
            vk::BufferUsageFlagBits2::eIndexBuffer));
    }

    fmt::println(
        stderr,
        "Uniform buffer size: {}",
        sizeof(Transform) * scene.meshes.size());

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        sceneBuffers.emplace_back(allocator.createBuffer(
            sizeof(Transform) * scene.meshes.size(),
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    }

    for (const auto &texture : scene.textures) {
        fmt::println(stderr, "uploading texture '{}'", texture.name.string());
        textureImageBuffers.emplace_back(allocator.createImageAndUploadData(
            cmd,
            texture.data,
            vk::ImageCreateInfo{
                {},
                vk::ImageType::e2D,
                vk::Format::eR8G8B8A8Srgb,
                // vk::Format::eR8G8B8A8Unorm,
                vk::Extent3D(texture.extent, 1),
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eSampled},
            vk::ImageLayout::eShaderReadOnlyOptimal));

        textureImageViews.emplace_back(device.createImageView(
            vk::ImageViewCreateInfo{
                {},
                textureImageBuffers.back().image,
                vk::ImageViewType::e2D,
                vk::Format::eR8G8B8A8Srgb,
                {},
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1}}));
    }

    endSingleTimeCommands(device, cmdPool, cmd, queue);

    return {
        primitiveBuffers,
        indexBuffers,
        sceneBuffers,
        textureImageBuffers,
        std::move(textureImageViews)};
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

auto createPipelineLayout(
    vk::raii::Device       &device,
    vk::DescriptorSetLayout descriptorSetLayout,
    uint32_t                maxFramesInFlight) -> vk::raii::PipelineLayout
{
    // descriptor set layout per frame
    auto descriptorSetLayouts = std::vector(maxFramesInFlight, descriptorSetLayout);

    // push constant range for transform index and texture index
    auto pushConstantRanges = std::array{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
        0,
        sizeof(PushConstantBlock)}};

    return {
        device,
        vk::PipelineLayoutCreateInfo{{}, descriptorSetLayouts, pushConstantRanges}};
}

auto submit( // TODO : move to Renderer
    vk::raii::CommandBuffer &cmdBuffer,
    vk::raii::Queue         &graphicsQueue,
    vk::Image                nextImage,
    vk::Semaphore            imageAvailableSemaphore,
    vk::Semaphore            renderFinishedSemaphore,
    vk::Semaphore            frameTimelineSemaphore,
    uint64_t                &timelineWaitValue,
    const uint32_t          &maxFramesInFlight) -> void
{
    // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
    cmdTransitionImageLayout(
        cmdBuffer,
        nextImage,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR);
    cmdBuffer.end();

    // end frame

    // prepare to submit the current frame for rendering
    // add the swapchain semaphore to wait for the image to be available

    uint64_t signalFrameValue = timelineWaitValue + maxFramesInFlight;
    timelineWaitValue         = signalFrameValue;

    auto waitSemaphoreSubmitInfo = vk::SemaphoreSubmitInfo{
        imageAvailableSemaphore,
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput};

    auto signalSemaphoreSubmitInfos = std::array{
        vk::SemaphoreSubmitInfo{
            renderFinishedSemaphore,
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput},
        vk::SemaphoreSubmitInfo{
            frameTimelineSemaphore,
            signalFrameValue,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput}};

    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{cmdBuffer};

    // fmt::print(stderr, "\n\nBefore vkQueueSubmit2()\n\n");
    graphicsQueue.submit2(
        vk::SubmitInfo2{
            {},
            waitSemaphoreSubmitInfo,
            commandBufferSubmitInfo,
            signalSemaphoreSubmitInfos});
    // fmt::print(stderr, "\n\nAfter vkQueueSubmit2()\n\n");
}
