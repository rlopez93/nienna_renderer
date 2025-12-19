#include "Renderer.hpp"
#include "Command.hpp"
#include "Utility.hpp"

#include <SDL3/SDL_video.h>

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <cstdint>

auto Renderer::getWindowExtent() const -> vk::Extent2D
{
    const vk::SurfaceCapabilitiesKHR caps =
        physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle);

    // If the surface dictates the extent, use it.
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }

    // Otherwise, we must derive it from the window framebuffer size and clamp.
    int fbWidth  = 0;
    int fbHeight = 0;
    SDL_GetWindowSizeInPixels(window.get(), &fbWidth, &fbHeight);

    // If minimized / not ready, return 0 extent; caller must handle it.
    if (fbWidth <= 0 || fbHeight <= 0) {
        return vk::Extent2D{0u, 0u};
    }

    vk::Extent2D extent{
        static_cast<uint32_t>(fbWidth),
        static_cast<uint32_t>(fbHeight)};

    extent.width =
        std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);

    extent.height = std::clamp(
        extent.height,
        caps.minImageExtent.height,
        caps.maxImageExtent.height);

    return extent;
}

auto Renderer::beginFrame() -> bool
{

    // begin frame
    // fmt::print(stderr, "\n\n<start rendering frame> <{}>\n\n", totalFrames);

    {
        // wait semaphore frame (n - numFrames)
        auto result = device.handle.waitSemaphores(
            vk::SemaphoreWaitInfo{{}, *timeline.semaphore, timeline.value()},
            std::numeric_limits<uint64_t>::max());
    }

    // reset current frame's command pool to reuse the command buffer
    timeline.pool().reset();
    timeline.buffer().begin({});

    auto result = swapchain.acquireNextImage();

    // FIXME:
    // Swapchain recreation logic
    if (result == vk::Result::eErrorOutOfDateKHR
        || result == vk::Result::eSuboptimalKHR) {
        swapchain.needRecreate = true;
    } else if (!(result == vk::Result::eSuccess
                 || result == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }

    if (swapchain.needRecreate) {
        return false;
    }

    return true;
}

auto Renderer::render(const SceneResources &sceneResources) -> void
{
    // color attachment image to render to: vk::RenderingAttachmentInfo
    auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
        swapchain.getNextImageView(),
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
        vk::AttachmentStoreOp::eDontCare,
        vk::ClearDepthStencilValue{1.0f, 0}};

    // rendering info for dynamic rendering
    auto renderingInfo = vk::RenderingInfo{
        {},
        vk::Rect2D{{}, getWindowExtent()},
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
        (!swapchain.imageInitialized[swapchain.nextImageIndex])
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
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,

        swapchain.getNextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    timeline.buffer().pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    swapchain.imageInitialized[swapchain.nextImageIndex] = true;

    timeline.buffer().beginRendering(renderingInfo);
    timeline.buffer().setViewportWithCount(
        vk::Viewport(
            0.0f,
            0.0f,
            getWindowExtent().width,
            getWindowExtent().height,
            0.0f,
            1.0f));
    timeline.buffer().setScissorWithCount(
        vk::Rect2D{vk::Offset2D(0, 0), getWindowExtent()});

    timeline.buffer().bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

    // bind texture resources passed to shader
    timeline.buffer().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *descriptors.pipelineLayout,
        0,
        *descriptors.descriptorSets[swapchain.frame.index],
        {});

    for (const auto &draw : sceneResources.draws) {
        // Current upload path is one vertex+index buffer per mesh => buffer index
        // equals draw item index. If you later batch/merge buffers, replace these
        // indices with draw.vertexBufferIndex / draw.indexBufferIndex.
        const uint32_t bufferIndex = draw.transformIndex;

        timeline.buffer().bindVertexBuffers(
            0,
            sceneResources.buffers.vertex[bufferIndex].buffer,
            {0});

        timeline.buffer().bindIndexBuffer(
            sceneResources.buffers.index[bufferIndex].buffer,
            0,
            vk::IndexType::eUint16);

        auto pushConstant = PushConstantBlock{
            .transformIndex  = draw.transformIndex,
            .textureIndex    = draw.textureIndex,
            .baseColorFactor = draw.baseColor};

        timeline.buffer().pushConstants2(
            vk::PushConstantsInfo{
                *descriptors.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(PushConstantBlock),
                &pushConstant});

        timeline.buffer()
            .drawIndexed(draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
    }

    timeline.buffer().endRendering();
}

Renderer::Renderer()
    : window{createWindow(
          windowWidth,
          windowHeight)},
      instance{},
      surface{
          instance,
          window},
      requiredExtensions{
          VK_KHR_SWAPCHAIN_EXTENSION_NAME,
          VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
          VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
          VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
          VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
          VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
          VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME,
          VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
          VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME,
          VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME,
          VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
          VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
          VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME

      },
      physicalDevice{
          instance,
          requiredExtensions},
      device{
          physicalDevice,
          window,
          surface,
          requiredExtensions},
      swapchain{
          device,
          physicalDevice,
          surface},
      // allocator{
      //     instance,
      //     physicalDevice,
      //     device},
      // depthFormat{findDepthFormat(physicalDevice.handle)},
      // depthImage{allocator.createImage(
      //     vk::ImageCreateInfo{
      //         {},
      //         vk::ImageType::e2D,
      //         depthFormat,
      //         vk::Extent3D(
      //             getWindowExtent(),
      //             1),
      //         1,
      //         1,
      //         vk::SampleCountFlagBits::e1,
      //         vk::ImageTiling::eOptimal,
      //         vk::ImageUsageFlagBits::eDepthStencilAttachment})},
      // depthImageView{device.handle.createImageView(
      //     vk::ImageViewCreateInfo{
      //         {},
      //         depthImage.image,
      //         vk::ImageViewType::e2D,
      //         depthFormat,
      //         {},
      //         vk::ImageSubresourceRange{
      //             vk::ImageAspectFlagBits::eDepth,
      //             0,
      //             1,
      //             0,
      //             1}})},
      // timeline{
      //     device,
      //     swapchain.frame.maxFramesInFlight},
      //
      poolSizes{
          {vk::DescriptorType::eSampler,
           1000},
          {vk::DescriptorType::eCombinedImageSampler,
           1000},
          {vk::DescriptorType::eSampledImage,
           1000},
          {vk::DescriptorType::eStorageImage,
           1000},
          {vk::DescriptorType::eUniformTexelBuffer,
           1000},
          {vk::DescriptorType::eStorageTexelBuffer,
           1000},
          {vk::DescriptorType::eUniformBuffer,
           1000},
          {vk::DescriptorType::eStorageBuffer,
           1000},
          {vk::DescriptorType::eUniformBufferDynamic,
           1000},
          {vk::DescriptorType::eStorageBufferDynamic,
           1000},
          {vk::DescriptorType::eInputAttachment,
           1000}},

      imguiDescriptorPool{
          device.handle,
          vk::DescriptorPoolCreateInfo{
              vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
              static_cast<uint32_t>(1000u * poolSizes.size()),
              poolSizes}}

{
    // after depthImage + depthImageView creation
    // create transient command pool for single-time commands
    auto transient = Command{device, vk::CommandPoolCreateFlagBits::eTransient};
    transient.beginSingleTime();

    cmdTransitionImageLayout(
        transient.buffer,
        depthImage.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eAttachmentOptimal,
        vk::ImageAspectFlagBits::eDepth);

    transient.endSingleTime(device);
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;  // Enable Gamepad Controls

    ImGui_ImplSDL3_InitForVulkan(window.get());

    ImGui_ImplVulkan_InitInfo init_info = {
        .ApiVersion          = VK_API_VERSION_1_4,
        .Instance            = *instance.handle,
        .PhysicalDevice      = *physicalDevice.handle,
        .Device              = *device.handle,
        .QueueFamily         = device.queueFamilyIndices.graphicsIndex,
        .Queue               = *device.graphicsQueue,
        .DescriptorPool      = *imguiDescriptorPool,
        .MinImageCount       = 3,
        .ImageCount          = 3,
        .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{{}, swapchain.imageFormat}};

    ImGui_ImplVulkan_Init(&init_info);
}
auto Renderer::drawFrame(
    Scene                             &scene,
    const std::chrono::duration<float> deltaTime,
    vk::raii::Pipeline                &pipeline,
    Descriptors                       &descriptors,
    bool                               framebufferResized) -> void
{
    // If minimized, skip rendering
    int w, h;
    SDL_GetWindowSizeInPixels(window.get(), &w, &h);

    if (w == 0 || h == 0) {
        SDL_WaitEvent(nullptr); // sleep until something happens
        return;
    }

    // FIXME:
    // Swapchain recreation logic

    // check swapchain rebuild
    if (framebufferResized || swapchain.needRecreate) {
        swapchain.recreate(device, physicalDevice, surface);

        depthImage = allocator.createImage(
            vk::ImageCreateInfo{
                {},
                vk::ImageType::e2D,
                depthFormat,
                vk::Extent3D(getWindowExtent(), 1),
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eDepthStencilAttachment});

        depthImageView = device.handle.createImageView(
            vk::ImageViewCreateInfo{
                {},
                depthImage.image,
                vk::ImageViewType::e2D,
                depthFormat,
                {},
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eDepth,
                    0,
                    1,
                    0,
                    1}});

        // after depthImage + depthImageView creation
        // create transient command pool for single-time commands
        auto transient = Command{device, vk::CommandPoolCreateFlagBits::eTransient};
        transient.beginSingleTime();

        cmdTransitionImageLayout(
            transient.buffer,
            depthImage.image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eAttachmentOptimal,
            vk::ImageAspectFlagBits::eDepth);

        transient.endSingleTime(device);
        framebufferResized = false;
        return;
    }

    // ImGui_ImplVulkan_NewFrame();
    // ImGui_ImplSDL3_NewFrame();
    // ImGui::NewFrame();

    // ImGui::ShowDemoWindow();

    // ImGui::Render();
    // auto imGuiDrawData = ImGui::GetDrawData();

    scene.update(deltaTime);

    // FIXME:
    // Does copying uniform data to device go in scene.update()?
    // Synchronization?

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
            scene.buffers.uniform[swapchain.frame.index].allocation,
            sizeof(Transform) * meshIndex,
            sizeof(Transform)));
    }
    auto light = Light{};

    VK_CHECK(vmaCopyMemoryToAllocation(
        allocator.allocator,
        &light,
        scene.buffers.light[swapchain.frame.index].allocation,
        0,
        sizeof(light)));

    beginFrame();

    // render scene
    render(scene, pipeline, descriptors);

    // render imGui
    // r.beginRender(
    //     vk::AttachmentLoadOp::eLoad,
    //     vk::AttachmentStoreOp::eStore,
    //     vk::AttachmentLoadOp::eLoad,
    //     vk::AttachmentStoreOp::eStore);

    // ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, *r.timeline.buffer());

    // r.endRender();

    submit();
    present();

    endFrame();
    // ImGui::EndFrame();

    // ++totalFrames;

    // if (totalFrames > 10) {
    //     break;
    // }
}
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

        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,

        swapchain.getNextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}};

    timeline.buffer().pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
    timeline.buffer().end();

    // end frame

    // prepare to submit the current frame for rendering
    // add the swapchain semaphore to wait for the image to be available

    uint64_t signalFrameValue = timeline.value() + swapchain.frame.maxFramesInFlight;
    timeline.value()          = signalFrameValue;

    auto waitSemaphoreSubmitInfo = vk::SemaphoreSubmitInfo{
        swapchain.getImageAvailableSemaphore(),
        {},
        vk::PipelineStageFlagBits2::eAllCommands};

    auto signalSemaphoreSubmitInfos = std::array{
        vk::SemaphoreSubmitInfo{
            swapchain.getRenderFinishedSemaphore(),
            {},
            vk::PipelineStageFlagBits2::eAllCommands},
        vk::SemaphoreSubmitInfo{
            timeline.semaphore,
            signalFrameValue,
            vk::PipelineStageFlagBits2::eAllCommands}};

    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{timeline.buffer()};

    device.graphicsQueue.submit2(
        vk::SubmitInfo2{
            {},
            waitSemaphoreSubmitInfo,
            commandBufferSubmitInfo,
            signalSemaphoreSubmitInfos});
}

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = swapchain.getRenderFinishedSemaphore();
    auto presentResult           = device.presentQueue.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *swapchain.handle,
            swapchain.nextImageIndex});

    // FIXME:
    // Swapchain recreation logic
    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        swapchain.needRecreate = true;
    }

    else if (!(presentResult == vk::Result::eSuccess
               || presentResult == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }
}
auto Renderer::endFrame() -> void
{
    // advance to the next frame
    swapchain.advance();
    timeline.advance();
}

Renderer::~Renderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
