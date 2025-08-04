#include "Renderer.hpp"
#include "App.hpp"
#include <ranges>

Renderer::Renderer()
    : window{createWindow(
          windowWidth,
          windowHeight)},
      instance{},
      surface{
          instance,
          window},
      physicalDevice{
          instance,
          surface},
      device{physicalDevice},
      graphicsQueue{
          device,
          QueueType::Graphics},
      presentQueue{
          device,
          QueueType::Present},
      swapchain{device},
      allocator{
          instance,
          physicalDevice,
          device},
      depthFormat{findDepthFormat(physicalDevice.handle)},
      depthImage{allocator.createImage(
          vk::ImageCreateInfo{
              {},
              vk::ImageType::e2D,
              depthFormat,
              vk::Extent3D(
                  getWindowExtent(),
                  1),
              1,
              1,
              vk::SampleCountFlagBits::e1,
              vk::ImageTiling::eOptimal,
              vk::ImageUsageFlagBits::eDepthStencilAttachment})},
      depthImageView{device.handle.createImageView(
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
                  1}})},
      timeline{
          device,
          graphicsQueue,
          swapchain.frame.maxFramesInFlight}

{
}

auto Renderer::submit() -> void
{

    // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
    cmdTransitionImageLayout(
        timeline.buffer(),
        swapchain.getNextImage(),
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::ePresentSrcKHR);

    timeline.buffer().end();

    // end frame

    // prepare to submit the current frame for rendering
    // add the swapchain semaphore to wait for the image to be available

    uint64_t signalFrameValue = timeline.value() + swapchain.frame.maxFramesInFlight;
    timeline.value()          = signalFrameValue;

    auto waitSemaphoreSubmitInfo = vk::SemaphoreSubmitInfo{
        swapchain.getImageAvailableSemaphore(),
        {},
        vk::PipelineStageFlagBits2::eColorAttachmentOutput};

    auto signalSemaphoreSubmitInfos = std::array{
        vk::SemaphoreSubmitInfo{
            swapchain.getRenderFinishedSemaphore(),
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput},
        vk::SemaphoreSubmitInfo{
            timeline.semaphore,
            signalFrameValue,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput}};

    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{timeline.buffer()};

    graphicsQueue.handle.submit2(
        vk::SubmitInfo2{
            {},
            waitSemaphoreSubmitInfo,
            commandBufferSubmitInfo,
            signalSemaphoreSubmitInfos});
}

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = swapchain.getRenderFinishedSemaphore();
    auto presentResult           = presentQueue.handle.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *swapchain.handle,
            swapchain.nextImageIndex});

    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        swapchain.needRecreate = true;
    }

    else if (!(presentResult == vk::Result::eSuccess
               || presentResult == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }
}

auto Renderer::getWindowExtent() const -> vk::Extent2D
{
    return physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle)
        .currentExtent;
}

auto Renderer::render(
    Scene              &scene,
    vk::raii::Pipeline &graphicsPipeline,
    Descriptors        &descriptors) -> void
{
    beginRender(
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare);

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

    for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
        // fmt::println(stderr, "scene.meshes[{}]", meshIndex);
        // bind vertex data
        timeline.buffer().bindVertexBuffers(
            0,
            scene.buffers.vertex[meshIndex].buffer,
            {0});

        // bind index data
        timeline.buffer().bindIndexBuffer(
            scene.buffers.index[meshIndex].buffer,
            0,
            vk::IndexType::eUint16);

        auto pushConstant = PushConstantBlock{
            .transformIndex  = static_cast<uint32_t>(meshIndex),
            .textureIndex    = static_cast<int32_t>(mesh.textureIndex.value_or(-1)),
            .baseColorFactor = mesh.color};

        timeline.buffer().pushConstants2(
            vk::PushConstantsInfo{
                *descriptors.pipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(PushConstantBlock),
                &pushConstant});

        // render draw call
        timeline.buffer()
            .drawIndexed(scene.meshes[meshIndex].indices.size(), 1, 0, 0, 0);
    }

    endRender();
}

auto Renderer::beginFrame() -> void
{

    // begin frame
    // fmt::print(stderr, "\n\n<start rendering frame> <{}>\n\n", totalFrames);

    // check swapchain rebuild
    if (swapchain.needRecreate) {
        swapchain.recreate(device, graphicsQueue);
    }

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

    if (result == vk::Result::eErrorOutOfDateKHR
        || result == vk::Result::eSuboptimalKHR) {
        swapchain.needRecreate = true;
    } else if (!(result == vk::Result::eSuccess
                 || result == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }

    if (swapchain.needRecreate) {
        return;
    }
}

auto Renderer::endFrame() -> void
{
    // advance to the next frame
    swapchain.advance();
    timeline.advance();
}
auto Renderer::beginRender(
    vk::AttachmentLoadOp  loadOp,
    vk::AttachmentStoreOp storeOp,
    vk::AttachmentLoadOp  depthLoadOp,
    vk::AttachmentStoreOp depthStoreOp) -> void
{
    // color attachment image to render to: vk::RenderingAttachmentInfo
    auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
        swapchain.getNextImageView(),
        vk::ImageLayout::eAttachmentOptimal,
        {},
        {},
        {},
        loadOp,
        storeOp,
        vk::ClearColorValue{std::array{0.2f, 0.5f, 1.0f, 1.0f}}};

    // depth attachment buffer: vk::RenderingAttachmentInfo
    auto renderingDepthAttachmentInfo = vk::RenderingAttachmentInfo{
        depthImageView,
        vk::ImageLayout::eAttachmentOptimal,
        {},
        {},
        {},
        depthLoadOp,
        depthStoreOp,
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
    cmdTransitionImageLayout(
        timeline.buffer(),
        swapchain.getNextImage(),
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eColorAttachmentOptimal);

    timeline.buffer().beginRendering(renderingInfo);
}

auto Renderer::endRender() -> void
{

    timeline.buffer().endRendering();
}
