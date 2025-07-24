#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_extension_inspection.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL_events.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include "spirv_reflect.h"
#include "stb_image.h"

#include <fmt/base.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numeric>
#include <ranges>
#include <variant>

#include "Allocator.hpp"
#include "Descriptor.hpp"
#include "Renderer.hpp"
#include "Shader.hpp"
#include "Utility.hpp"
#include "gltfLoader.hpp"

auto createPipeline(vk::raii::Device &device, const std::filesystem::path &shaderPath,
                    vk::Format imageFormat, vk::Format depthFormat,
                    vk::raii::PipelineLayout &pipelineLayout) -> vk::raii::Pipeline
{
    auto       shaderModule = createShaderModule(device, shaderPath);
    // The stages used by this pipeline
    const auto shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eVertex, shaderModule, "vertexMain", {}},
        vk::PipelineShaderStageCreateInfo{
            {}, vk::ShaderStageFlagBits::eFragment, shaderModule, "fragmentMain", {}},
    };

    const auto vertexBindingDescriptions = std::array{vk::VertexInputBindingDescription{
        0, sizeof(Primitive), vk::VertexInputRate::eVertex}};

    const auto vertexAttributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{0, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Primitive, position)},

        vk::VertexInputAttributeDescription{1, 0, vk::Format::eR32G32B32Sfloat,
                                            offsetof(Primitive, normal)},
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Primitive, uv)},
        vk::VertexInputAttributeDescription{3, 0, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(Primitive, color)},
    };

    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        {}, vertexBindingDescriptions, vertexAttributeDescriptions};

    const auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {}, vk::PrimitiveTopology::eTriangleList, false};

    const auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{};

    const auto pipelineRasterizationStateCreateInfo =
        vk::PipelineRasterizationStateCreateInfo{{},
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
            {}, true, true, vk::CompareOp::eLessOrEqual};

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
            {}, {}, vk::LogicOp::eCopy, 1, &colorBlendAttachmentState};

    const auto dynamicStates = std::array{vk::DynamicState::eViewportWithCount,
                                          vk::DynamicState::eScissorWithCount};

    const auto pipelineDynamicStateCreateInfo =
        vk::PipelineDynamicStateCreateInfo{{}, dynamicStates};

    auto pipelineRenderingCreateInfo =
        vk::PipelineRenderingCreateInfo{{}, imageFormat, depthFormat};

    auto graphicsPipelineCreateInfo =
        vk::GraphicsPipelineCreateInfo{{},
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

auto createBuffers(vk::raii::Device &device, vk::raii::CommandPool &cmdPool,
                   Allocator &allocator, vk::raii::Queue &queue, const Scene &scene,
                   uint64_t maxFramesInFlight)
    -> std::tuple<std::vector<Buffer>, std::vector<Buffer>, std::vector<Buffer>,
                  std::vector<Image>, std::vector<vk::raii::ImageView>>
{
    auto primitiveBuffers    = std::vector<Buffer>{};
    auto indexBuffers        = std::vector<Buffer>{};
    auto sceneBuffers        = std::vector<Buffer>{};
    auto textureImageBuffers = std::vector<Image>{};
    auto textureImageViews   = std::vector<vk::raii::ImageView>{};

    auto cmd = beginSingleTimeCommands(device, cmdPool);

    for (const auto &mesh : scene.meshes) {
        primitiveBuffers.emplace_back(allocator.createBufferAndUploadData(
            cmd, mesh.primitives, vk::BufferUsageFlagBits2::eVertexBuffer));

        indexBuffers.emplace_back(allocator.createBufferAndUploadData(
            cmd, mesh.indices, vk::BufferUsageFlagBits2::eIndexBuffer));
    }

    fmt::println(stderr, "Uniform buffer size: {}",
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
            cmd, texture.data,
            vk::ImageCreateInfo{{},
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

        textureImageViews.emplace_back(device.createImageView(vk::ImageViewCreateInfo{
            {},
            textureImageBuffers.back().image,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Srgb,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}}));
    }

    endSingleTimeCommands(device, cmdPool, cmd, queue);

    return {primitiveBuffers, indexBuffers, sceneBuffers, textureImageBuffers,
            std::move(textureImageViews)};
}

auto main(int argc, char **argv) -> int
{
    auto filePath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "third_party/glTF-Sample-Assets/Models/Duck/glTF/Duck.gltf";
        } else {
            return std::string{argv[1]};
        }
    }();

    auto gltfDirectory = filePath.parent_path();
    auto gltfFilename  = filePath.filename();

    auto shaderPath = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "build/_autogen/mesh-refactor.slang.spv";
        } else {
            return std::string{argv[2]};
        }
    }();

    Renderer  r;
    Allocator allocator{r.ctx.instance, r.ctx.physicalDevice, r.ctx.device,
                        vk::ApiVersion14};

    // get swapchain images
    auto swapchainImages     = r.ctx.swapchain.getImages();
    auto swapchainImageViews = std::vector<vk::raii::ImageView>{};
    auto maxFramesInFlight   = swapchainImages.size();

    for (auto image : swapchainImages) {
        swapchainImageViews.emplace_back(
            r.ctx.device,
            vk::ImageViewCreateInfo{{},
                                    image,
                                    vk::ImageViewType::e2D,
                                    r.ctx.imageFormat,
                                    {},
                                    {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
    }

    auto imageAvailableSemaphores = std::vector<vk::raii::Semaphore>{};
    auto renderFinishedSemaphores = std::vector<vk::raii::Semaphore>{};

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        imageAvailableSemaphores.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
        renderFinishedSemaphores.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
    }

    auto depthImage = allocator.createImage(
        vk::ImageCreateInfo{{},
                            vk::ImageType::e2D,
                            r.ctx.depthFormat,
                            vk::Extent3D(r.ctx.windowExtent, 1),
                            1,
                            1

        }
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    auto depthImageView = r.ctx.device.createImageView(vk::ImageViewCreateInfo{
        {},
        depthImage.image,
        vk::ImageViewType::e2D,
        r.ctx.depthFormat,
        {},
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});

    // create transient command pool for single-time commands
    auto transientCommandPool = vk::raii::CommandPool{
        r.ctx.device,
        {vk::CommandPoolCreateFlagBits::eTransient, r.ctx.graphicsQueueIndex}};

    // Transition image layout
    auto commandBuffer = beginSingleTimeCommands(r.ctx.device, transientCommandPool);

    for (auto image : swapchainImages) {
        cmdTransitionImageLayout(commandBuffer, image, vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::ePresentSrcKHR,
                                 vk::ImageAspectFlagBits::eColor);
    }

    endSingleTimeCommands(r.ctx.device, transientCommandPool, commandBuffer,
                          r.ctx.graphicsQueue);

    auto commandPools            = std::vector<vk::raii::CommandPool>{};
    auto commandBuffers          = std::vector<vk::raii::CommandBuffer>{};
    auto timelineSemaphoreValues = std::vector<uint64_t>(maxFramesInFlight, 0);
    std::ranges::iota(timelineSemaphoreValues, 0);

    uint64_t initialValue = maxFramesInFlight - 1;

    auto timelineSemaphoreCreateInfo =
        vk::SemaphoreTypeCreateInfo{vk::SemaphoreType::eTimeline, initialValue};
    auto frameTimelineSemaphore =
        vk::raii::Semaphore{r.ctx.device, {{}, &timelineSemaphoreCreateInfo}};

    for (auto n : std::views::iota(0u, maxFramesInFlight)) {
        commandPools.emplace_back(
            r.ctx.device, vk::CommandPoolCreateInfo{{}, r.ctx.graphicsQueueIndex});

        commandBuffers.emplace_back(std::move(vk::raii::CommandBuffers{
            r.ctx.device,
            vk::CommandBufferAllocateInfo{commandPools.back(),
                                          vk::CommandBufferLevel::ePrimary, 1}}
                                                  .front()));
    }

    auto asset = getGltfAsset(gltfDirectory / gltfFilename);
    auto scene = getSceneData(asset, gltfDirectory);

    auto sampler = vk::raii::Sampler{r.ctx.device, vk::SamplerCreateInfo{}};

    auto [primitiveBuffers, indexBuffers, sceneBuffers, textureImageBuffers,
          textureImageViews] =
        createBuffers(r.ctx.device, transientCommandPool, allocator,
                      r.ctx.graphicsQueue, scene, maxFramesInFlight);

    uint32_t textureCount = textureImageBuffers.size();
    uint32_t meshCount    = scene.meshes.size();
    fmt::println(stderr, "textures.size(): {}", textureCount);

    Descriptors descriptors{r.ctx.device, meshCount, textureCount, maxFramesInFlight};

    // for (const auto &textureImageView : textureImageViews) {
    // for (const auto &[sceneBuffer, descriptorSet] :
    // std::views::zip(sceneBuffers, descriptorSets)) {
    for (auto frameIndex : std::views::iota(0u, maxFramesInFlight)) {
        auto descriptorWrites = std::vector<vk::WriteDescriptorSet>{};

        auto descriptorBufferInfos = std::vector<vk::DescriptorBufferInfo>{};

        vk::DeviceSize transformBufferSize = sizeof(Transform) * meshCount;
        for (auto meshIndex : std::views::iota(0, static_cast<int32_t>(meshCount))) {
            descriptorBufferInfos.emplace_back(sceneBuffers[frameIndex].buffer,
                                               sizeof(Transform) * meshIndex,
                                               sizeof(Transform));
        }

        if (!descriptorBufferInfos.empty()) {
            descriptorWrites.emplace_back(
                vk::WriteDescriptorSet{descriptors.descriptorSets[frameIndex],
                                       0,
                                       0,
                                       vk::DescriptorType::eUniformBuffer,
                                       {},
                                       descriptorBufferInfos});
        }

        auto descriptorImageInfos = std::vector<vk::DescriptorImageInfo>{};
        for (auto k : std::views::iota(0u, textureCount)) {
            descriptorImageInfos.emplace_back(sampler, *textureImageViews[k],
                                              vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        if (!descriptorImageInfos.empty()) {
            descriptorWrites.emplace_back(descriptors.descriptorSets[frameIndex], 1, 0,
                                          vk::DescriptorType::eCombinedImageSampler,
                                          descriptorImageInfos);
        }

        r.ctx.device.updateDescriptorSets(descriptorWrites, {});
    }

    // descriptor set layout per frame
    auto descriptorSetLayouts =
        std::vector(maxFramesInFlight, *descriptors.descriptorSetLayout);

    assert(descriptors.descriptorSets.size() == descriptorSetLayouts.size());
    fmt::println(stderr, "pass");

    // push constant range for transform index and texture index
    auto pushConstantRanges = std::array{vk::PushConstantRange{
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, 8}};

    auto pipelineLayout = vk::raii::PipelineLayout{
        r.ctx.device,
        vk::PipelineLayoutCreateInfo{{}, descriptorSetLayouts, pushConstantRanges}};

    auto graphicsPipeline = createPipeline(r.ctx.device, shaderPath, r.ctx.imageFormat,
                                           r.ctx.depthFormat, pipelineLayout);

    uint32_t frameRingCurrent = 0;
    uint32_t currentFrame     = 0;
    uint32_t totalFrames      = 0;

    bool swapchainNeedRebuild = false;

    using namespace std::literals;

    auto           start        = std::chrono::high_resolution_clock::now();
    auto           previousTime = start;
    auto           currentTime  = start;
    auto           runningTime  = 0.0s;
    bool           running      = true;
    constexpr auto period       = 1.0s;

    SDL_Event e;
    while (running) {
        currentTime = std::chrono::high_resolution_clock::now();
        runningTime += currentTime - previousTime;

        if (runningTime > period) {
            auto fps =
                totalFrames
                / std::chrono::duration_cast<std::chrono::seconds>(currentTime - start)
                      .count();
            fmt::println(stderr, "{} fps", fps);
            runningTime -= period;
        }

        previousTime = currentTime;

        if (currentFrame != frameRingCurrent) {
            fmt::println(stderr, "frame:{}, frame ring:{}", currentFrame,
                         frameRingCurrent);
        }

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // begin frame
        // fmt::print(stderr, "\n\n<start rendering frame> <{}>\n\n", totalFrames);

        // check swapchain rebuild
        if (swapchainNeedRebuild) {
            r.recreateSwapchain(r.ctx.graphicsQueue, currentFrame, swapchainImages,
                                swapchainImageViews, imageAvailableSemaphores,
                                renderFinishedSemaphores);
            swapchainNeedRebuild = false;
        }

        // get current frame data
        auto &cmdPool           = commandPools[frameRingCurrent];
        auto &cmdBuffer         = commandBuffers[frameRingCurrent];
        auto  timelineWaitValue = timelineSemaphoreValues[frameRingCurrent];

        {
            // wait semaphore frame (n - numFrames)
            auto result = r.ctx.device.waitSemaphores(
                vk::SemaphoreWaitInfo{{}, *frameTimelineSemaphore, timelineWaitValue},
                std::numeric_limits<uint64_t>::max());
        }

        // reset current frame's command pool to reuse the command buffer
        cmdPool.reset();
        cmdBuffer.begin({});

        auto [result, nextImageIndex] =
            r.ctx.swapchain.acquireNextImage(std::numeric_limits<uint64_t>::max(),
                                             imageAvailableSemaphores[currentFrame]);

        if (result == vk::Result::eErrorOutOfDateKHR
            || result == vk::Result::eSuboptimalKHR) {
            swapchainNeedRebuild = true;
        } else if (!(result == vk::Result::eSuccess
                     || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        if (swapchainNeedRebuild) {
            continue;
        }

        // update uniform buffers

        for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
            auto transform =
                Transform{.modelMatrix          = mesh.modelMatrix,
                          .viewProjectionMatrix = scene.viewProjectionMatrix};
            // transform.viewProjectionMatrix[1][1] *= -1;

            VK_CHECK(vmaCopyMemoryToAllocation(
                allocator.allocator, &transform, sceneBuffers[currentFrame].allocation,
                sizeof(Transform) * meshIndex, sizeof(Transform)));
        }

        // color attachment image to render to: vk::RenderingAttachmentInfo
        auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
            swapchainImageViews[nextImageIndex],
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearColorValue{std::array{0.0f, 0.0f, 0.0f, 1.0f}}};

        // depth attachment buffer: vk::RenderingAttachmentInfo
        auto renderingDepthAttachmentInfo =
            vk::RenderingAttachmentInfo{depthImageView,
                                        vk::ImageLayout::eAttachmentOptimal,
                                        {},
                                        {},
                                        {},
                                        vk::AttachmentLoadOp::eClear,
                                        vk::AttachmentStoreOp::eStore,
                                        vk::ClearDepthStencilValue{1.0f, 0}};

        // rendering info for dynamic rendering
        auto renderingInfo = vk::RenderingInfo{
            {}, vk::Rect2D{{}, r.ctx.windowExtent}, 1,
            {}, renderingColorAttachmentInfo,       &renderingDepthAttachmentInfo};

        // transition swapchain image layout
        cmdTransitionImageLayout(cmdBuffer, swapchainImages[nextImageIndex],
                                 vk::ImageLayout::eUndefined,
                                 vk::ImageLayout::eColorAttachmentOptimal);

        cmdBuffer.beginRendering(renderingInfo);

        cmdBuffer.setViewportWithCount(
            vk::Viewport(0.0f, 0.0f, r.ctx.windowExtent.width,
                         r.ctx.windowExtent.height, 0.0f, 1.0f));
        cmdBuffer.setScissorWithCount(
            vk::Rect2D{vk::Offset2D(0, 0), r.ctx.windowExtent});

        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        // fmt::println(stderr, "bindDescriptorSets2(), frame {}", currentFrame);
        // bind texture resources passed to shader
        cmdBuffer.bindDescriptorSets2(vk::BindDescriptorSetsInfo{
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
            pipelineLayout, 0, *descriptors.descriptorSets[currentFrame]});

        for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
            // fmt::println(stderr, "scene.meshes[{}]", meshIndex);
            // bind vertex data
            cmdBuffer.bindVertexBuffers(0, primitiveBuffers[meshIndex].buffer, {0});

            // bind index data
            cmdBuffer.bindIndexBuffer(indexBuffers[meshIndex].buffer, 0,
                                      vk::IndexType::eUint16);

            struct Index {
                uint32_t transformIndex;
                int32_t  textureIndex;
            } index{.transformIndex = static_cast<uint32_t>(meshIndex),
                    .textureIndex =
                        static_cast<int32_t>(mesh.textureIndex.value_or(-1))};

            cmdBuffer.pushConstants2(vk::PushConstantsInfo{
                *pipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0, sizeof(Index), &index

            });

            // render draw call
            cmdBuffer.drawIndexed(scene.meshes[meshIndex].indices.size(), 1, 0, 0, 0);
        }

        cmdBuffer.endRendering();
        // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
        cmdTransitionImageLayout(cmdBuffer, swapchainImages[nextImageIndex],
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::ePresentSrcKHR);
        cmdBuffer.end();

        // end frame

        // prepare to submit the current frame for rendering
        // add the swapchain semaphore to wait for the image to be available

        uint64_t signalFrameValue = timelineWaitValue + maxFramesInFlight;
        timelineSemaphoreValues[frameRingCurrent] = signalFrameValue;

        auto waitSemaphoreSubmitInfo =
            vk::SemaphoreSubmitInfo{imageAvailableSemaphores[currentFrame],
                                    {},
                                    vk::PipelineStageFlagBits2::eColorAttachmentOutput};

        auto signalSemaphoreSubmitInfos = std::array{
            vk::SemaphoreSubmitInfo{renderFinishedSemaphores[nextImageIndex],
                                    {},
                                    vk::PipelineStageFlagBits2::eColorAttachmentOutput},
            vk::SemaphoreSubmitInfo{
                frameTimelineSemaphore, signalFrameValue,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput}};

        auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{cmdBuffer};

        // fmt::print(stderr, "\n\nBefore vkQueueSubmit2()\n\n");
        r.ctx.graphicsQueue.submit2(vk::SubmitInfo2{{},
                                                    waitSemaphoreSubmitInfo,
                                                    commandBufferSubmitInfo,
                                                    signalSemaphoreSubmitInfos});
        // fmt::print(stderr, "\n\nAfter vkQueueSubmit2()\n\n");

        // present frame
        auto presentResult = r.ctx.presentQueue.presentKHR(
            vk::PresentInfoKHR{*renderFinishedSemaphores[nextImageIndex],
                               *r.ctx.swapchain, nextImageIndex});

        if (presentResult == vk::Result::eErrorOutOfDateKHR
            || result == vk::Result::eSuboptimalKHR) {
            swapchainNeedRebuild = true;
        }

        else if (!(result == vk::Result::eSuccess
                   || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        // advance to the next frame in the swapchain
        currentFrame = (currentFrame + 1) % maxFramesInFlight;

        // move to the next frame
        frameRingCurrent = (frameRingCurrent + 1) % maxFramesInFlight;

        // fmt::print(
        //     stderr,
        //     "imageIndex: {}, currentFrame: {}, frameRingCurrent: {},
        //     totalFrames:
        //     {}\n", nextImageIndex, currentFrame, nextImageIndex,
        //     totalFrames);
        //
        ++totalFrames;
    }

    r.ctx.device.waitIdle();

    return 0;
}
