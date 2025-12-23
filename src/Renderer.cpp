#include "Renderer.hpp"
#include "Pipeline.hpp"
#include "PipelineLayout.hpp"
#include "SceneRenderData.hpp"

auto Renderer::createDescriptorPool(
    Device                           &device,
    const ShaderInterfaceDescription &shaderInterfaceDescription,
    uint32_t                          maxFramesInFlight) -> vk::raii::DescriptorPool
{
    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(shaderInterfaceDescription.bindings.size());

    uint32_t maxSets = maxFramesInFlight;

    for (const auto &binding : shaderInterfaceDescription.bindings) {

        poolSizes.emplace_back(binding.type, binding.count * maxFramesInFlight);
    }

    return vk::raii::DescriptorPool{
        device.handle,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            maxSets,
            static_cast<uint32_t>(poolSizes.size()),
            poolSizes.data()}};
}

auto Renderer::allocateFrameDescriptorSets() -> void
{
    frames.descriptorSets.clear();
    frames.descriptorSets.reserve(frames.descriptorSets.capacity());

    auto layouts = std::vector<vk::DescriptorSetLayout>(
        frames.maxFrames(),
        *shaderInterface.handle);

    vk::DescriptorSetAllocateInfo allocInfo{*descriptorPool, layouts};
    auto sets = context.device.handle.allocateDescriptorSets(allocInfo);

    for (auto &set : sets) {
        frames.descriptorSets.emplace_back(std::move(set));
    }
}

auto Renderer::initializePerFrameUniforms(
    Allocator &allocator,
    uint32_t   meshCount) -> void
{
    frames.createPerFrameUniformBuffers(allocator, meshCount);
}
Renderer::Renderer(
    RenderContext        &context_,
    const RendererConfig &config)
    : context{context_},
      shaderInterface{
          context.device,
          config.shaderInterfaceDescription},
      descriptorPool{createDescriptorPool(
          context.device,
          config.shaderInterfaceDescription,
          config.maxFramesInFlight)},
      pipelineLayout{createPipelineLayout(
          context.device.handle,
          {*shaderInterface.handle})},
      graphicsPipeline{createPipeline(
          context.device,
          config.shaderPath,
          config.colorFormat,
          config.depthFormat,
          pipelineLayout)},
      frames{
          context.device,
          config.maxFramesInFlight}
{
    allocateFrameDescriptorSets();
}

auto Renderer::beginFrame() -> bool
{
    if (context.swapchain.needRecreate) {
        context.recreateRenderTargets();
    }

    const auto &timelineSemaphore = frames.timelineSemaphore();
    const auto &timelineValue     = frames.timelineValue();
    [[maybe_unused]]
    auto waitResult = context.device.handle.waitSemaphores(
        vk::SemaphoreWaitInfo{{}, timelineSemaphore, timelineValue},
        std::numeric_limits<uint64_t>::max());

    frames.cmdPool().reset();
    frames.cmd().begin({});

    auto acquireResult =
        context.swapchain.acquireNextImage(frames.imageAvailableSemaphore());

    if (acquireResult == vk::Result::eErrorOutOfDateKHR
        || acquireResult == vk::Result::eSuboptimalKHR) {
        context.swapchain.needRecreate = true;
        return false;
    }

    if (acquireResult != vk::Result::eSuccess) {
        context.swapchain.needRecreate = true;
        return false;
    }

    return true;
}

auto Renderer::render(const SceneRenderData &sceneRenderData) -> void
{
    // color attachment image to render to: vk::RenderingAttachmentInfo
    auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
        context.swapchain.nextImageView(),
        vk::ImageLayout::eAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ClearColorValue{std::array{0.2f, 0.5f, 1.0f, 1.0f}}};

    // depth attachment buffer: vk::RenderingAttachmentInfo
    auto renderingDepthAttachmentInfo = vk::RenderingAttachmentInfo{
        context.depth.view,
        vk::ImageLayout::eAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ClearDepthStencilValue{1.0f, 0}};

    // rendering info for dynamic rendering
    auto renderingInfo = vk::RenderingInfo{
        {},
        vk::Rect2D{{}, context.extent()},
        1,
        {},
        renderingColorAttachmentInfo,
        &renderingDepthAttachmentInfo};

    // transition swapchain image layout
    // cmdTransitionImageLayout(
    //     timeline.buffer(),
    //     swapchain.getNextImage(),
    //     vk::ImageLayout::eUndefined,
    //     vk::ImageLayout::eColorAttachmentOptimal);

    auto oldLayout = vk::ImageLayout{
        (!context.swapchain.imageInitialized[context.swapchain.nextImageIndex])
            ? vk::ImageLayout::eUndefined
            : vk::ImageLayout::ePresentSrcKHR};

    vk::ImageMemoryBarrier2 barrier{
        // src: be conservative; assume last use was as a color attachment write
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,

        // dst: color attachment read/write during rendering
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentRead
            | vk::AccessFlagBits2::eColorAttachmentWrite,

        // layout transition
        oldLayout,
        vk::ImageLayout::eColorAttachmentOptimal,

        // queue family ownership unchanged
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,

        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    frames.cmd().pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    context.swapchain.imageInitialized[context.swapchain.nextImageIndex] = true;

    frames.cmd().beginRendering(renderingInfo);
    frames.cmd().setViewportWithCount(
        vk::Viewport(
            0.0f,
            0.0f,
            context.extent().width,
            context.extent().height,
            0.0f,
            1.0f));
    frames.cmd().setScissorWithCount(vk::Rect2D{vk::Offset2D(0, 0), context.extent()});

    frames.cmd().bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

    // bind texture resources passed to shader
    frames.cmd().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        frames.currentDescriptorSet(),
        {});

    for (const auto &draw : sceneRenderData.draws) {
        // fmt::println("I am here on frame {}.", frames.current());
        // Current upload path is one vertex+index buffer per mesh => buffer index
        // equals draw item index. If you later batch/merge buffers, replace these
        // indices with draw.vertexBufferIndex / draw.indexBufferIndex.
        const uint32_t bufferIndex = draw.transformIndex;

        frames.cmd().bindVertexBuffers(
            0,
            sceneRenderData.vertexBuffers[bufferIndex].buffer,
            {0});

        frames.cmd().bindIndexBuffer(
            sceneRenderData.indexBuffers[bufferIndex].buffer,
            0,
            vk::IndexType::eUint16);

        auto pushConstant = PushConstantBlock{
            .transformIndex  = draw.transformIndex,
            .textureIndex    = draw.textureIndex,
            .baseColorFactor = draw.baseColor};

        frames.cmd().pushConstants2(
            vk::PushConstantsInfo{
                *pipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(PushConstantBlock),
                &pushConstant});

        frames.cmd()
            .drawIndexed(draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
    }

    frames.cmd().endRendering();
}

// Renderer::Renderer()
//     : window{createWindow(
//           windowWidth,
//           windowHeight)},
//       instance{},
//       surface{
//           instance,
//           window},
//       requiredExtensions{
//           VK_KHR_SWAPCHAIN_EXTENSION_NAME,
//           VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
//           VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
//           VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
//           VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
//           VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
//           VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME,
//           VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
//           VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME,
//           VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME,
//           VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
//           VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
//           VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
//
//       },
//       physicalDevice{
//           instance,
//           requiredExtensions},
//       device{
//           physicalDevice,
//           window,
//           surface,
//           requiredExtensions},
//       frames{
//           device,
//           3},
//       swapchain{
//           device,
//           physicalDevice,
//           surface},
//     pipelineLayout(createPipelineLayout(device.handle, resourceLayouts),
//                    graphicsPipeline{
//     createPipeline(device, shaderPath, imageFormat, depthFormat, pipelineLayout)}
//
//         // allocator{
//         //     instance,
//         //     physicalDevice,
//         //     device},
//         // depthFormat{findDepthFormat(physicalDevice.handle)},
//         // depthImage{allocator.createImage(
//         //     vk::ImageCreateInfo{
//         //         {},
//         //         vk::ImageType::e2D,
//         //         depthFormat,
//         //         vk::Extent3D(
//         //             getWindowExtent(),
//         //             1),
//         //         1,
//         //         1,
//         //         vk::SampleCountFlagBits::e1,
//         //         vk::ImageTiling::eOptimal,
//         //         vk::ImageUsageFlagBits::eDepthStencilAttachment})},
//         // depthImageView{device.handle.createImageView(
//         //     vk::ImageViewCreateInfo{
//         //         {},
//         //         depthImage.image,
//         //         vk::ImageViewType::e2D,
//         //         depthFormat,
//         //         {},
//         //         vk::ImageSubresourceRange{
//         //             vk::ImageAspectFlagBits::eDepth,
//         //             0,
//         //             1,
//         //             0,
//         //             1}})},
//         // timeline{
//         //     device,
//         //     swapchain.frame.maxFramesInFlight},
//         //
//         imguiPoolSizes{
//             {vk::DescriptorType::eSampler, 1000},
//             {vk::DescriptorType::eCombinedImageSampler, 1000},
//             {vk::DescriptorType::eSampledImage, 1000},
//             {vk::DescriptorType::eStorageImage, 1000},
//             {vk::DescriptorType::eUniformTexelBuffer, 1000},
//             {vk::DescriptorType::eStorageTexelBuffer, 1000},
//             {vk::DescriptorType::eUniformBuffer, 1000},
//             {vk::DescriptorType::eStorageBuffer, 1000},
//             {vk::DescriptorType::eUniformBufferDynamic, 1000},
//             {vk::DescriptorType::eStorageBufferDynamic, 1000},
//             {vk::DescriptorType::eInputAttachment, 1000}},
//
//         imguiDescriptorPool{
//             device.handle,
//             vk::DescriptorPoolCreateInfo{
//                 vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
//                 static_cast<uint32_t>(1000u * poolSizes.size()),
//                 poolSizes}}
//
//     {
//     // after depthImage + depthImageView creation
//     // create transient command pool for single-time commands
//     auto transient = Command{device, vk::CommandPoolCreateFlagBits::eTransient};
//     transient.beginSingleTime();
//
//     cmdTransitionImageLayout(
//         transient.buffer,
//         depthImage.image,
//         vk::ImageLayout::eUndefined,
//         vk::ImageLayout::eAttachmentOptimal,
//         vk::ImageAspectFlagBits::eDepth);
//
//     transient.endSingleTime(device);
//     // Setup Dear ImGui context
//     IMGUI_CHECKVERSION();
//     ImGui::CreateContext();
//     ImGuiIO &io = ImGui::GetIO();
//     (void)io;
//     io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard
//     Controls io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable
//     Gamepad Controls
//
//     ImGui_ImplSDL3_InitForVulkan(window.get());
//
//     ImGui_ImplVulkan_InitInfo init_info = {
//         .ApiVersion          = VK_API_VERSION_1_4,
//         .Instance            = *instance.handle,
//         .PhysicalDevice      = *physicalDevice.handle,
//         .Device              = *device.handle,
//         .QueueFamily         = device.queueFamilyIndices.graphicsIndex,
//         .Queue               = *device.graphicsQueue,
//         .DescriptorPool      = *imguiDescriptorPool,
//         .MinImageCount       = 3,
//         .ImageCount          = 3,
//         .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
//         .UseDynamicRendering = true,
//         .PipelineRenderingCreateInfo =
//             vk::PipelineRenderingCreateInfo{{}, swapchain.imageFormat}};
//
//     ImGui_ImplVulkan_Init(&init_info);
//     }
//
// auto Renderer::drawFrame(
//     Scene                             &scene,
//     const std::chrono::duration<float> deltaTime,
//     vk::raii::Pipeline                &pipeline,
//     Descriptors                       &descriptors,
//     bool                               framebufferResized) -> void
// {
//     // If minimized, skip rendering
//     int w, h;
//     SDL_GetWindowSizeInPixels(window.get(), &w, &h);
//
//     if (w == 0 || h == 0) {
//         SDL_WaitEvent(nullptr); // sleep until something happens
//         return;
//     }
//
//     // FIXME:
//     // Swapchain recreation logic
//
//     // check swapchain rebuild
//     if (framebufferResized || swapchain.needRecreate) {
//         swapchain.recreate(device, physicalDevice, surface);
//
//         depthImage = allocator.createImage(
//             vk::ImageCreateInfo{
//                 {},
//                 vk::ImageType::e2D,
//                 depthFormat,
//                 vk::Extent3D(getWindowExtent(), 1),
//                 1,
//                 1,
//                 vk::SampleCountFlagBits::e1,
//                 vk::ImageTiling::eOptimal,
//                 vk::ImageUsageFlagBits::eDepthStencilAttachment});
//
//         depthImageView = device.handle.createImageView(
//             vk::ImageViewCreateInfo{
//                 {},
//                 depthImage.image,
//                 vk::ImageViewType::e2D,
//                 depthFormat,
//                 {},
//                 vk::ImageSubresourceRange{
//                     vk::ImageAspectFlagBits::eDepth,
//                     0,
//                     1,
//                     0,
//                     1}});
//
//         // after depthImage + depthImageView creation
//         // create transient command pool for single-time commands
//         auto transient = Command{device,
//         vk::CommandPoolCreateFlagBits::eTransient}; transient.beginSingleTime();
//
//         cmdTransitionImageLayout(
//             transient.buffer,
//             depthImage.image,
//             vk::ImageLayout::eUndefined,
//             vk::ImageLayout::eAttachmentOptimal,
//             vk::ImageAspectFlagBits::eDepth);
//
//         transient.endSingleTime(device);
//         framebufferResized = false;
//         return;
//     }
//
//     // ImGui_ImplVulkan_NewFrame();
//     // ImGui_ImplSDL3_NewFrame();
//     // ImGui::NewFrame();
//
//     // ImGui::ShowDemoWindow();
//
//     // ImGui::Render();
//     // auto imGuiDrawData = ImGui::GetDrawData();
//
//     scene.update(deltaTime);
//
//     // FIXME:
//     // Does copying uniform data to device go in scene.update()?
//     // Synchronization?
//
//     // update uniform buffers
//     for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
//         auto transform = Transform{
//             .modelMatrix          = mesh.modelMatrix,
//             .viewProjectionMatrix = scene.getCamera().getProjectionMatrix()
//                                   * scene.getCamera().getViewMatrix()};
//         // transform.viewProjectionMatrix[1][1] *= -1;
//
//         VK_CHECK(vmaCopyMemoryToAllocation(
//             allocator.allocator,
//             &transform,
//             scene.buffers.uniform[swapchain.frame.index].allocation,
//             sizeof(Transform) * meshIndex,
//             sizeof(Transform)));
//     }
//     auto light = Light{};
//
//     VK_CHECK(vmaCopyMemoryToAllocation(
//         allocator.allocator,
//         &light,
//         scene.buffers.light[swapchain.frame.index].allocation,
//         0,
//         sizeof(light)));
//
//     beginFrame();
//
//     // render scene
//     render(scene, pipeline, descriptors);
//
//     // render imGui
//     // r.beginRender(
//     //     vk::AttachmentLoadOp::eLoad,
//     //     vk::AttachmentStoreOp::eStore,
//     //     vk::AttachmentLoadOp::eLoad,
//     //     vk::AttachmentStoreOp::eStore);
//
//     // ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, *r.timeline.buffer());
//
//     // r.endRender();
//
//     submit();
//     present();
//
//     endFrame();
//     // ImGui::EndFrame();
//
//     // ++totalFrames;
//
//     // if (totalFrames > 10) {
//     //     break;
//     // }
// }
auto Renderer::submit() -> void
{
    // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
    // cmdTransitionImageLayout(
    //     timeline.buffer(),
    //     swapchain.getNextImage(),
    //     vk::ImageLayout::eColorAttachmentOptimal,
    //     vk::ImageLayout::ePresentSrcKHR);

    vk::ImageMemoryBarrier2 barrier{
        // src: rendering wrote the color attachment
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,

        // dst: presentation engine
        vk::PipelineStageFlagBits2::eBottomOfPipe,
        vk::AccessFlagBits2::eNone,

        // layout transition
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR,

        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,

        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    frames.cmd().pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
    frames.cmd().end();

    // end frame

    // prepare to submit the current frame for rendering
    // add the swapchain semaphore to wait for the image to be available

    uint64_t signalFrameValue = frames.timelineValue() + frames.maxFrames();
    frames.timelineValue()    = signalFrameValue;

    auto waitSemaphoreSubmitInfo = vk::SemaphoreSubmitInfo{
        frames.imageAvailableSemaphore(),
        {},
        vk::PipelineStageFlagBits2::eAllCommands};

    auto signalSemaphoreSubmitInfos = std::array{
        vk::SemaphoreSubmitInfo{
            context.swapchain.renderFinishedSemaphore(),
            {},
            vk::PipelineStageFlagBits2::eAllCommands},
        vk::SemaphoreSubmitInfo{
            frames.timelineSemaphore(),
            signalFrameValue,
            vk::PipelineStageFlagBits2::eAllCommands}};

    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{frames.cmd()};

    context.device.graphicsQueue.submit2(
        vk::SubmitInfo2{
            {},
            waitSemaphoreSubmitInfo,
            commandBufferSubmitInfo,
            signalSemaphoreSubmitInfos});
}

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = context.swapchain.renderFinishedSemaphore();
    auto presentResult           = context.device.presentQueue.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *context.swapchain.handle,
            context.swapchain.nextImageIndex});

    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        context.swapchain.needRecreate = true;
    }
}

auto Renderer::endFrame() -> void
{
    submit();
    present();
    // advance to the next frame
    frames.advance();
}

vk::DescriptorSet Renderer::currentDescriptorSet() const
{
    return frames.currentDescriptorSet();
}

Buffer &Renderer::currentTransformUBO()
{
    return frames.transformUBO[frames.current()];
}

Buffer &Renderer::currentLightUBO()
{
    return frames.lightUBO[frames.current()];
}

// Renderer::~Renderer()
// {
//     ImGui_ImplVulkan_Shutdown();
//     ImGui_ImplSDL3_Shutdown();
//     ImGui::DestroyContext();
// }
