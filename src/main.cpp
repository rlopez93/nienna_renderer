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
#include "Renderer.hpp"
#include "Shader.hpp"
#include "Utility.hpp"
#include "gltfLoader.hpp"

auto createPipeline(vk::raii::Device &device, vk::Format imageFormat,
                    vk::Format depthFormat, vk::raii::PipelineLayout &pipelineLayout)
    -> vk::raii::Pipeline
{
    auto       shaderModule = createShaderModule(device);
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
        vk::VertexInputAttributeDescription{2, 0, vk::Format::eR32G32B32A32Sfloat,
                                            offsetof(Primitive, tangent)},
        vk::VertexInputAttributeDescription{4, 0, vk::Format::eR32G32Sfloat,
                                            offsetof(Primitive, uv)},
        vk::VertexInputAttributeDescription{5, 0, vk::Format::eR32G32B32A32Sfloat,
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
                   Allocator &allocator, vk::raii::Queue &queue, const Scene &scene)
{
    auto cmd = beginSingleTimeCommands(device, cmdPool);

    std::vector<Primitive> primitivesData;
    std::vector<uint16_t>  indicesData;

    vk::DeviceSize primitiveOffset = 0;
    vk::DeviceSize indicesOffset   = 0;
    for (const auto &mesh : scene.meshes) {
        primitivesData.append_range(mesh.primitives);
        primitiveOffset += sizeof(Primitive) * mesh.primitives.size();

        indicesData.append_range(mesh.indices | std::ranges::transform());
        indicesOffset += sizeof(uint16_t) * mesh.indices.size();
    }

    auto primitivesBuffer = allocator.createBufferAndUploadData(
        cmd, primitivesData, vk::BufferUsageFlagBits2::eVertexBuffer);

    auto indicesBuffer = allocator.createBufferAndUploadData(
        cmd, indicesData, vk::BufferUsageFlagBits2::eIndexBuffer);

    endSingleTimeCommands(device, cmdPool, cmd, queue);
}

// auto uploadBuffers(
//     vk::raii::Device                 &device,
//     vk::raii::CommandPool            &transientCommandPool,
//     const std::vector<Vertex>        &vertexData,
//     const std::vector<uint16_t>      &indexData,
//     const std::vector<unsigned char> &textureData,
//     const vk::Extent2D                textureExtent,
//     vk::raii::Queue                  &queue,
//     Allocator                        &allocator)
//     -> std::tuple<
//         Buffer,
//         Buffer,
//         Image,
//         vk::raii::ImageView>
// {
//     auto commandBuffer = beginSingleTimeCommands(device, transientCommandPool);
//
//     // fmt::print(stderr, "\n\nvertexBuffer = createBufferAndUploadData()\n\n");
//     auto vertexBuffer = allocator.createBufferAndUploadData(
//         commandBuffer,
//         vertexData,
//         vk::BufferUsageFlagBits2::eVertexBuffer
//             | vk::BufferUsageFlagBits2::eStorageBuffer);
//     // fmt::print(stderr, "\n\nindexBuffer = createBufferAndUploadData()\n\n");
//     auto indexBuffer = allocator.createBufferAndUploadData(
//         commandBuffer,
//         indexData,
//         vk::BufferUsageFlagBits2::eIndexBuffer);
//     // fmt::print(
//     // stderr,
//     // "texture extent: {}x{}\n",
//     // textureExtent.width,
//     // textureExtent.height);
//     // fmt::print(stderr, "\n\ntextureImage = createImageAndUploadData()\n\n");
//     auto textureImage = allocator.createImageAndUploadData(
//         commandBuffer,
//         textureData,
//         vk::ImageCreateInfo{
//             {},
//             vk::ImageType::e2D,
//             vk::Format::eR8G8B8A8Srgb,
//             // vk::Format::eR8G8B8A8Unorm,
//             {static_cast<uint32_t>(textureExtent.width),
//              static_cast<uint32_t>(textureExtent.height),
//              1},
//             1,
//             1,
//             vk::SampleCountFlagBits::e1,
//             vk::ImageTiling::eOptimal,
//             vk::ImageUsageFlagBits::eSampled},
//         vk::ImageLayout::eShaderReadOnlyOptimal);
//     // fmt::print(stderr, "\n\nimageView = createImageView()\n\n");
//     auto imageView = device.createImageView(
//         vk::ImageViewCreateInfo{
//             {},
//             textureImage.image,
//             vk::ImageViewType::e2D,
//             vk::Format::eR8G8B8A8Srgb,
//             {},
//             vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0,
//             1}});
//     endSingleTimeCommands(device, transientCommandPool, commandBuffer, queue);
//
//     return {vertexBuffer, indexBuffer, textureImage, std::move(imageView)};
// }

auto createDescriptorSetLayout(vk::raii::Device &device, uint64_t maxFramesInFlight)
    -> vk::raii::DescriptorSetLayout
{
    const auto descriptorSetLayoutBindings = std::array{
        vk::DescriptorSetLayoutBinding{0, vk::DescriptorType::eUniformBuffer, 1,
                                       vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{1, vk::DescriptorType::eCombinedImageSampler, 1,
                                       vk::ShaderStageFlagBits::eFragment}};

    return {device, vk::DescriptorSetLayoutCreateInfo{{}, descriptorSetLayoutBindings}};
}

auto createDescriptorPool(vk::raii::Device &device, uint64_t maxFramesInFlight)
    -> vk::raii::DescriptorPool
{
    auto poolSizes =
        std::array{vk::DescriptorPoolSize{vk::DescriptorType::eUniformBuffer,
                                          static_cast<uint32_t>(maxFramesInFlight)},
                   vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler,
                                          static_cast<uint32_t>(maxFramesInFlight)}};

    return {device, vk::DescriptorPoolCreateInfo{
                        vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
                        static_cast<uint32_t>(maxFramesInFlight), poolSizes}};
}

auto createDescriptorSets(vk::raii::Device              &device,
                          vk::raii::DescriptorPool      &descriptorPool,
                          vk::raii::DescriptorSetLayout &descriptorSetLayout,
                          uint64_t                       maxFramesInFlight)
    -> std::vector<vk::raii::DescriptorSet>
{

    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto descriptorSetAllocateInfo =
        vk::DescriptorSetAllocateInfo{descriptorPool, descriptorSetLayouts};

    return device.allocateDescriptorSets(descriptorSetAllocateInfo);
}

auto main(int argc, char **argv) -> int
{
    auto filePath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "resources/Duck/glTF/Duck.gltf";
        } else {
            return std::string{argv[1]};
        }
    }();

    auto gltfDirectory = filePath.parent_path();
    auto gltfFilename  = filePath.filename();
    auto shaderPath    = std::filesystem::path("shaders/shader.vert.spv");

    Renderer  r;
    Allocator allocator{r.ctx.instance, r.ctx.physicalDevice, r.ctx.device,
                        vk::ApiVersion14};

    // get swapchain images
    auto swapchainImages     = r.ctx.swapchain.getImages();
    auto swapchainImageViews = std::vector<vk::raii::ImageView>{};
    auto maxFramesInFlight   = swapchainImages.size();

    auto imageAvailable = std::vector<vk::raii::Semaphore>{};
    auto renderFinished = std::vector<vk::raii::Semaphore>{};

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

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        imageAvailable.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
        renderFinished.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
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

    auto commandPools   = std::vector<vk::raii::CommandPool>{};
    auto commandBuffers = std::vector<vk::raii::CommandBuffer>{};
    auto frameNumbers   = std::vector<uint64_t>(maxFramesInFlight, 0);
    std::ranges::iota(frameNumbers, 0);

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

    // TODO create vertex/index buffers
    // TODO create texture image buffers
    // TODO create uniform buffers

    auto descriptorSetLayout =
        createDescriptorSetLayout(r.ctx.device, maxFramesInFlight);

    auto descriptorPool = createDescriptorPool(r.ctx.device, maxFramesInFlight);

    auto descriptorSets = createDescriptorSets(r.ctx.device, descriptorPool,
                                               descriptorSetLayout, maxFramesInFlight);

    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto pipelineLayout = vk::raii::PipelineLayout{
        r.ctx.device, vk::PipelineLayoutCreateInfo{{}, descriptorSetLayouts}};

    auto graphicsPipeline = createPipeline(r.ctx.device, r.ctx.imageFormat,
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
        }

        // get current frame data
        auto &cmdPool   = commandPools[frameRingCurrent];
        auto &cmdBuffer = commandBuffers[frameRingCurrent];
        auto  waitValue = frameNumbers[frameRingCurrent];

        {
            // wait semaphore frame (n - numFrames)
            auto result = r.ctx.device.waitSemaphores(
                vk::SemaphoreWaitInfo{{}, *frameTimelineSemaphore, waitValue},
                std::numeric_limits<uint64_t>::max());
        }

        // reset current frame's command pool to reuse the command buffer
        cmdPool.reset();
        cmdBuffer.begin({});

        auto [result, nextImageIndex] = r.ctx.swapchain.acquireNextImage(
            std::numeric_limits<uint64_t>::max(), imageAvailable[currentFrame]);

        if (result == vk::Result::eErrorOutOfDateKHR) {
            swapchainNeedRebuild = true;
        } else if (!(result == vk::Result::eSuccess
                     || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        if (swapchainNeedRebuild) {
            // continue;
        }

        // update uniform buffers
        // auto scene = Transform{
        //     .model      = modelMatrix,
        //     .view       = viewMatrix,
        //     .projection = projectionMatrix};

        // scene.projection[1][1] *= -1;

        // vmaCopyMemoryToAllocation(
        //     allocator.allocator,
        //     &scene,
        //     sceneBuffers[currentFrame].allocation,
        //     0,
        //     sizeof(SceneInfo));

        // color attachment image to render to: vk::RenderingAttachmentInfo
        auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
            swapchainImageViews[nextImageIndex],
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearColorValue{std::array{0.0f, 0.0f, 1.0f, 1.0f}}};

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

        // vk::RenderingInfo
        auto renderingInfo = vk::RenderingInfo{
            {}, vk::Rect2D{{}, r.ctx.windowExtent}, 1,
            {}, renderingColorAttachmentInfo,       &renderingDepthAttachmentInfo};

        // transition swapchain image layout:

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

        // bind texture resources passed to shader
        cmdBuffer.bindDescriptorSets2(vk::BindDescriptorSetsInfo{
            vk::ShaderStageFlagBits::eFragment, pipelineLayout, 0,
            *descriptorSets[currentFrame]});

        // cmdBuffer.bindVertexBuffers(uint32_t firstBinding, const ArrayProxy<const
        // vk::Buffer> &buffers, const ArrayProxy<const vk::DeviceSize> &offsets)

        // bind vertex data
        // cmdBuffer.bindVertexBuffers(0, vertexBuffer.buffer, {0});

        // bind index data
        // cmdBuffer.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint16);

        // cmdBuffer.drawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t
        // firstIndex, int32_t vertexOffset, uint32_t firstInstance)
        // cmdBuffer.drawIndexed(indexData.size(), 1, 0, 0, 0);

        cmdBuffer.endRendering();

        // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
        cmdTransitionImageLayout(cmdBuffer, swapchainImages[nextImageIndex],
                                 vk::ImageLayout::eColorAttachmentOptimal,
                                 vk::ImageLayout::ePresentSrcKHR);
        cmdBuffer.end();

        // end frame

        // prepare to submit the current frame for rendering
        // add the swapchain semaphore to wait for the image to be available

        uint64_t signalFrameValue      = waitValue + maxFramesInFlight;
        frameNumbers[frameRingCurrent] = signalFrameValue;

        auto waitSemaphore =
            vk::SemaphoreSubmitInfo{imageAvailable[currentFrame],
                                    {},
                                    vk::PipelineStageFlagBits2::eColorAttachmentOutput};

        auto signalSemaphores = std::array{
            vk::SemaphoreSubmitInfo{renderFinished[nextImageIndex],
                                    {},
                                    vk::PipelineStageFlagBits2::eColorAttachmentOutput},
            vk::SemaphoreSubmitInfo{
                frameTimelineSemaphore, signalFrameValue,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput}};

        auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{cmdBuffer};

        // fmt::print(stderr, "\n\nBefore vkQueueSubmit2()\n\n");
        r.ctx.graphicsQueue.submit2(vk::SubmitInfo2{
            {}, waitSemaphore, commandBufferSubmitInfo, signalSemaphores});
        // fmt::print(stderr, "\n\nAfter vkQueueSubmit2()\n\n");

        // present frame
        auto presentResult = r.ctx.presentQueue.presentKHR(vk::PresentInfoKHR{
            *renderFinished[nextImageIndex], *r.ctx.swapchain, nextImageIndex});

        if (presentResult == vk::Result::eErrorOutOfDateKHR) {
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
        //     "imageIndex: {}, currentFrame: {}, frameRingCurrent: {}, totalFrames:
        //     {}\n", nextImageIndex, currentFrame, nextImageIndex, totalFrames);
        //
        ++totalFrames;
    }

    r.ctx.device.waitIdle();

    return 0;
}
// auto [vertexBuffer, indexBuffer, textureImage, textureImageView] = uploadBuffers(
//     r.ctx.device,
//     transientCommandPool,
//     vertexData,
//     indexData,
//     textureData,
//     textureExtent,
//     r.ctx.graphicsQueue,
//     allocator);

// auto sceneBuffers = std::vector<Buffer>{};
//
// for (auto i : std::views::iota(0u, maxFramesInFlight)) {
//     sceneBuffers.emplace_back(allocator.createBuffer(
//         sizeof(SceneInfo),
//         vk::BufferUsageFlagBits2::eUniformBuffer
//             | vk::BufferUsageFlagBits2::eTransferDst,
//         VMA_MEMORY_USAGE_AUTO,
//         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
//             | VMA_ALLOCATION_CREATE_MAPPED_BIT));
// }
//
// auto descriptorSetLayout = createDescriptorSetLayout(r.ctx.device,
// maxFramesInFlight);
//
// auto descriptorPool = createDescriptorPool(r.ctx.device, maxFramesInFlight);
//
// auto descriptorSets = createDescriptorSets(
//     r.ctx.device, descriptorPool, descriptorSetLayout, maxFramesInFlight);
// for (auto i : std::views::iota(0u, maxFramesInFlight)) {
//     auto descriptorBufferInfo =
//         vk::DescriptorBufferInfo{sceneBuffers[i].buffer, 0, sizeof(SceneInfo)};
//     auto descriptorImageInfo = vk::DescriptorImageInfo{
//         sampler,
//         textureImageView,
//         vk::ImageLayout::eShaderReadOnlyOptimal};
//     auto descriptorWrites = std::array{
//         vk::WriteDescriptorSet{
//             descriptorSets[i],
//             0,
//             0,
//             1,
//             vk::DescriptorType::eUniformBuffer,
//             {},
//             &descriptorBufferInfo},
//         vk::WriteDescriptorSet{
//             descriptorSets[i],
//             1,
//             0,
//             1,
//             vk::DescriptorType::eCombinedImageSampler,
//             &descriptorImageInfo}};
//     r.ctx.device.updateDescriptorSets(descriptorWrites, {});
// }
//
