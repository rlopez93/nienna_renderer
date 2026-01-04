#include "Renderer.hpp"
#include "DebugView.hpp"
#include "Pipeline.hpp"
#include "PipelineLayout.hpp"
#include "SceneRenderData.hpp"
#include "ShaderInterfaceTypes.hpp"

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

auto Renderer::initializePerFrameUniformBuffers(
    Allocator &allocator,
    uint32_t   nodeInstancesCount) -> void
{
    frames.initializePerFrameUniformBuffers(allocator, nodeInstancesCount);
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
        const auto oldSwapImgs = context.swapchain.images;
        const auto oldRtImgs   = context.renderTargets.images();

        context.recreateRenderTargets();

        imageLayoutState.forgetImages(oldSwapImgs);
        imageLayoutState.forgetImages(oldRtImgs);
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
    auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
        context.swapchain.nextImageView(),
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eStore,
        vk::ClearColorValue{
            std::array{0.2f, 0.5f, 1.0f, 1.0f},
        },
    };

    auto &depth = context.renderTargets.mainDepth;

    auto renderingDepthAttachmentInfo = vk::RenderingAttachmentInfo{
        depth.view,
        vk::ImageLayout::eDepthAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eClear,
        vk::AttachmentStoreOp::eDontCare,
        vk::ClearDepthStencilValue{1.0f, 0},
    };

    auto renderingInfo = vk::RenderingInfo{
        {},
        vk::Rect2D{{}, context.extent()},
        1,
        {},
        renderingColorAttachmentInfo,
        &renderingDepthAttachmentInfo,
    };

    imageLayoutState.transition(
        frames.cmd(),
        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{
            vk::ImageAspectFlagBits::eColor,
            0,
            1,
            0,
            1,
        },
        ImageUse::kColorAttachmentWrite);

    imageLayoutState.transition(
        frames.cmd(),
        depth.image.get(),
        depth.range(),
        ImageUse::kDepthAttachmentWrite);

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

    if (debugView == DebugView::Wireframe) {
        frames.cmd().setPolygonModeEXT(vk::PolygonMode::eLine);
    }

    else {
        frames.cmd().setPolygonModeEXT(vk::PolygonMode::eFill);
    }

    frames.cmd().bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

    // bind texture resources passed to shader
    frames.cmd().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        frames.currentDescriptorSet(),
        {});

    for (const auto &draw : sceneRenderData.draws) {

        frames.cmd().bindVertexBuffers(
            0,
            sceneRenderData.vertexBuffers[draw.geometryIndex].buffer,
            {0});

        frames.cmd().bindIndexBuffer(
            sceneRenderData.indexBuffers[draw.geometryIndex].buffer,
            0,
            vk::IndexType::eUint32);

        auto pushConstant = PushConstants{
            .nodeInstanceIndex = draw.nodeInstanceIndex,
            .materialIndex     = draw.materialIndex,
            .debugView         = static_cast<uint32_t>(debugView)};

        frames.cmd().pushConstants2(
            vk::PushConstantsInfo{
                *pipelineLayout,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment,
                0,
                sizeof(PushConstants),
                &pushConstant});

        frames.cmd()
            .drawIndexed(draw.indexCount, 1, draw.firstIndex, draw.vertexOffset, 0);
    }

    frames.cmd().endRendering();
}

auto Renderer::submit() -> void
{
    imageLayoutState.transition(
        frames.cmd(),
        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        ImageUse::kPresent);

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

Buffer &Renderer::currentFrameUBO()
{
    return frames.frameUBO[frames.current()];
}

Buffer &Renderer::currentNodeInstancesSSBO()
{
    return frames.nodeInstancesSSBO[frames.current()];
}

void Renderer::cycleDebugView()
{
    switch (debugView) {
    case DebugView::Shaded:
        debugView = DebugView::Normals;
        break;
    case DebugView::Normals:
        debugView = DebugView::Wireframe;
        break;
    case DebugView::Wireframe:
        debugView = DebugView::Shaded;
        break;
    }
}
