#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan_raii.hpp>

#include "App.hpp"
#include "Renderer.hpp"

#include <SDL3/SDL_events.h>
#include <fmt/base.h>

#include <numeric>
#include <ranges>

auto main(
    int    argc,
    char **argv) -> int
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
    Allocator allocator{
        r.ctx.instance,
        r.ctx.physicalDevice,
        r.ctx.device,
        vk::ApiVersion14};

    auto depthImage = allocator.createImage(
        vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            r.ctx.depthFormat,
            vk::Extent3D(r.ctx.windowExtent, 1),
            1,
            1}
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    auto depthImageView = r.ctx.device.createImageView(
        vk::ImageViewCreateInfo{
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

    for (auto image : r.ctx.swapchain.images) {
        cmdTransitionImageLayout(
            commandBuffer,
            image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageAspectFlagBits::eColor);
    }

    endSingleTimeCommands(
        r.ctx.device,
        transientCommandPool,
        commandBuffer,
        r.ctx.graphicsQueue);

    auto commandPools   = std::vector<vk::raii::CommandPool>{};
    auto commandBuffers = std::vector<vk::raii::CommandBuffer>{};
    auto timelineSemaphoreValues =
        std::vector<uint64_t>(r.ctx.swapchain.frame.maxFramesInFlight, 0);
    std::ranges::iota(timelineSemaphoreValues, 0);

    uint64_t initialValue = r.ctx.swapchain.frame.maxFramesInFlight - 1;

    auto timelineSemaphoreCreateInfo =
        vk::SemaphoreTypeCreateInfo{vk::SemaphoreType::eTimeline, initialValue};
    auto frameTimelineSemaphore =
        vk::raii::Semaphore{r.ctx.device, {{}, &timelineSemaphoreCreateInfo}};

    for (auto n : std::views::iota(0u, r.ctx.swapchain.frame.maxFramesInFlight)) {
        commandPools.emplace_back(
            r.ctx.device,
            vk::CommandPoolCreateInfo{{}, r.ctx.graphicsQueueIndex});

        commandBuffers.emplace_back(
            std::move(
                vk::raii::CommandBuffers{
                    r.ctx.device,
                    vk::CommandBufferAllocateInfo{
                        commandPools.back(),
                        vk::CommandBufferLevel::ePrimary,
                        1}}
                    .front()));
    }

    auto asset = getGltfAsset(gltfDirectory / gltfFilename);
    auto scene = getSceneData(asset, gltfDirectory);

    auto sampler = vk::raii::Sampler{r.ctx.device, vk::SamplerCreateInfo{}};

    auto
        [primitiveBuffers,
         indexBuffers,
         sceneBuffers,
         textureImageBuffers,
         textureImageViews] =
            createBuffers(
                r.ctx.device,
                transientCommandPool,
                allocator,
                r.ctx.graphicsQueue,
                scene,
                r.ctx.swapchain.frame.maxFramesInFlight);

    uint32_t textureCount = textureImageBuffers.size();
    uint32_t meshCount    = scene.meshes.size();
    fmt::println(stderr, "textures.size(): {}", textureCount);
    fmt::println(stderr, "meshes.size(): {}", meshCount);

    auto descriptors = Descriptors{
        r.ctx.device,
        meshCount,
        textureCount,
        r.ctx.swapchain.frame.maxFramesInFlight};

    updateDescriptorSets(
        r.ctx.device,
        descriptors,
        sceneBuffers,
        meshCount,
        textureImageViews,
        sampler);

    auto pipelineLayout = createPipelineLayout(
        r.ctx.device,
        descriptors.descriptorSetLayout,
        r.ctx.swapchain.frame.maxFramesInFlight);

    auto graphicsPipeline = createPipeline(
        r.ctx.device,
        shaderPath,
        r.ctx.swapchain.imageFormat,
        r.ctx.depthFormat,
        pipelineLayout);

    uint32_t frameRingCurrent = 0;
    uint32_t totalFrames      = 0;

    using namespace std::literals;

    auto           start        = std::chrono::high_resolution_clock::now();
    auto           previousTime = start;
    auto           currentTime  = start;
    auto           runningTime  = 0.0s;
    bool           running      = true;
    constexpr auto period       = 1.0s;

    SDL_Event e;
    while (running) {
        currentTime    = std::chrono::high_resolution_clock::now();
        auto deltaTime = currentTime - previousTime;
        runningTime += deltaTime;

        if (runningTime > period) {
            auto fps =
                totalFrames
                / std::chrono::duration_cast<std::chrono::seconds>(currentTime - start)
                      .count();
            fmt::println(stderr, "{} fps", fps);
            runningTime -= period;
        }

        previousTime = currentTime;

        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
            scene.processInput(e);
            scene.getCamera().processInput(e);
        }

        scene.getCamera().update(deltaTime);

        // begin frame
        // fmt::print(stderr, "\n\n<start rendering frame> <{}>\n\n", totalFrames);

        // check swapchain rebuild
        if (r.ctx.swapchain.needRecreate) {
            r.ctx.swapchain.recreate(
                r.ctx.device,
                r.ctx.vkbDevice,
                r.ctx.graphicsQueue);
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

        auto result = r.ctx.swapchain.acquireNextImage();

        if (result == vk::Result::eErrorOutOfDateKHR
            || result == vk::Result::eSuboptimalKHR) {
            r.ctx.swapchain.needRecreate = true;
        } else if (!(result == vk::Result::eSuccess
                     || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        if (r.ctx.swapchain.needRecreate) {
            continue;
        }

        // update uniform buffers

        for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
            auto transform = Transform{
                .modelMatrix          = mesh.modelMatrix,
                .viewProjectionMatrix = scene.getCamera().getProjectionMatrix()
                                      * scene.getCamera().getViewMatrix()};
            // transform.viewProjectionMatrix[1][1] *= -1;

            VK_CHECK(vmaCopyMemoryToAllocation(
                allocator.allocator,
                &transform,
                sceneBuffers[r.ctx.swapchain.frame.currentFrameIndex].allocation,
                sizeof(Transform) * meshIndex,
                sizeof(Transform)));
        }

        // color attachment image to render to: vk::RenderingAttachmentInfo
        auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
            r.ctx.swapchain.getNextImageView(),
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearColorValue{std::array{0.2f, 0.5f, 1.0f, 1.0f}}};

        // depth attachment buffer: vk::RenderingAttachmentInfo
        auto renderingDepthAttachmentInfo = vk::RenderingAttachmentInfo{
            depthImageView,
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearDepthStencilValue{1.0f, 0}};

        // rendering info for dynamic rendering
        auto renderingInfo = vk::RenderingInfo{
            {},
            vk::Rect2D{{}, r.ctx.windowExtent},
            1,
            {},
            renderingColorAttachmentInfo,
            &renderingDepthAttachmentInfo};

        // transition swapchain image layout
        cmdTransitionImageLayout(
            cmdBuffer,
            r.ctx.swapchain.getNextImage(),
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal);

        cmdBuffer.beginRendering(renderingInfo);

        cmdBuffer.setViewportWithCount(
            vk::Viewport(
                0.0f,
                0.0f,
                r.ctx.windowExtent.width,
                r.ctx.windowExtent.height,
                0.0f,
                1.0f));
        cmdBuffer.setScissorWithCount(
            vk::Rect2D{vk::Offset2D(0, 0), r.ctx.windowExtent});

        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        // fmt::println(stderr, "bindDescriptorSets2(), frame {}", currentFrame);
        // bind texture resources passed to shader
        cmdBuffer.bindDescriptorSets2(
            vk::BindDescriptorSetsInfo{
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                pipelineLayout,
                0,
                *descriptors.descriptorSets[r.ctx.swapchain.frame.currentFrameIndex]});

        for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
            // fmt::println(stderr, "scene.meshes[{}]", meshIndex);
            // bind vertex data
            cmdBuffer.bindVertexBuffers(0, primitiveBuffers[meshIndex].buffer, {0});

            // bind index data
            cmdBuffer.bindIndexBuffer(
                indexBuffers[meshIndex].buffer,
                0,
                vk::IndexType::eUint16);

            auto pushConstant = PushConstantBlock{
                .transformIndex  = static_cast<uint32_t>(meshIndex),
                .textureIndex    = static_cast<int32_t>(mesh.textureIndex.value_or(-1)),
                .baseColorFactor = mesh.color};

            cmdBuffer.pushConstants2(
                vk::PushConstantsInfo{
                    *pipelineLayout,
                    vk::ShaderStageFlagBits::eVertex
                        | vk::ShaderStageFlagBits::eFragment,
                    0,
                    sizeof(PushConstantBlock),
                    &pushConstant});

            // render draw call
            cmdBuffer.drawIndexed(scene.meshes[meshIndex].indices.size(), 1, 0, 0, 0);
        }

        cmdBuffer.endRendering();

        submit(
            cmdBuffer,
            r.ctx.graphicsQueue,
            r.ctx.swapchain.getNextImage(),
            r.ctx.swapchain.getImageAvailableSemaphore(),
            r.ctx.swapchain.getRenderFinishedSemaphore(),
            frameTimelineSemaphore,
            timelineSemaphoreValues[frameRingCurrent],
            r.ctx.swapchain.frame.maxFramesInFlight);

        // present frame
        r.present();

        frameRingCurrent =
            (frameRingCurrent + 1) % r.ctx.swapchain.frame.maxFramesInFlight;

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
