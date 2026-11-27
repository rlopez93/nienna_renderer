#include "Renderer.hpp"
#include "DebugView.hpp"
#include "Pipeline.hpp"
#include "PipelineLayout.hpp"
#include "RenderableResources.hpp"
#include "ShaderInterfaceTypes.hpp"

auto Renderer::createDescriptorPool(
    Device&                           device,
    ShaderInterfaceDescription const& shaderInterfaceDescription,
    uint32_t                          maxFramesInFlight) -> vk::raii::DescriptorPool
{
    std::vector<vk::DescriptorPoolSize> poolSizes;
    poolSizes.reserve(shaderInterfaceDescription.bindings.size());

    uint32_t maxSets = maxFramesInFlight;

    for (auto const& binding : shaderInterfaceDescription.bindings) {

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

    for (auto& set : sets) {
        frames.descriptorSets.emplace_back(std::move(set));
    }
}

auto Renderer::initializePerFrameUniformBuffers(
    Allocator& allocator,
    uint32_t   nodeInstancesCount) -> void
{
    frames.initializePerFrameUniformBuffers(allocator, nodeInstancesCount);
}

Renderer::Renderer(
    RenderContext&        context_,
    RendererConfig const& config)
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
          {{*shaderInterface.handle}})},
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
        auto const oldSwapImgs = context.swapchain.images;
        auto const oldRtImgs   = context.renderTargets.images();

        context.recreateRenderTargets();

        imageLayoutState.forgetImages(oldSwapImgs);
        imageLayoutState.forgetImages(oldRtImgs);
    }

    auto const& timelineSemaphore = frames.timelineSemaphore();
    auto const& timelineValue     = frames.timelineValue();
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

auto Renderer::render(RenderableResources const& renderableResources) -> void
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

    auto& depth = context.renderTargets.mainDepth;

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

    frames.cmd().bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        *pipelineLayout,
        0,
        frames.currentDescriptorSet(),
        {});

    for (auto const& draw : renderableResources.draws) {

        frames.cmd().bindVertexBuffers(
            0,
            renderableResources.vertexBuffers[draw.geometryIndex].buffer,
            {0});

        frames.cmd().bindIndexBuffer(
            renderableResources.indexBuffers[draw.geometryIndex].buffer,
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

auto Renderer::beginUiPass() -> void
{
    // No implicit ordering between render passes (spec: note under vkCmdEndRendering)
    auto const uiPassBarrier = vk::ImageMemoryBarrier2{
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,
        vk::AccessFlagBits2::eColorAttachmentRead
            | vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::ImageLayout::eColorAttachmentOptimal,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
    };

    frames.cmd().pipelineBarrier2(
        vk::DependencyInfo{}.setImageMemoryBarriers(uiPassBarrier));

    auto const colorAttachmentInfo = vk::RenderingAttachmentInfo{
        context.swapchain.nextImageView(),
        vk::ImageLayout::eColorAttachmentOptimal,
        {},
        {},
        {},
        vk::AttachmentLoadOp::eLoad,
        vk::AttachmentStoreOp::eStore,
    };

    auto const renderingInfo = vk::RenderingInfo{
        {},
        vk::Rect2D{{}, context.extent()},
        1,
        {},
        colorAttachmentInfo,
    };

    frames.cmd().beginRendering(renderingInfo);
}

auto Renderer::endUiPass() -> void
{
    frames.cmd().endRendering();
}

auto Renderer::commandBuffer() -> vk::raii::CommandBuffer&
{
    return frames.cmd();
}

auto Renderer::submit() -> void
{
    imageLayoutState.transition(
        frames.cmd(),
        context.swapchain.nextImage(),
        vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        ImageUse::kPresent);

    frames.cmd().end();

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
    frames.advance();
}

auto Renderer::currentDescriptorSet() const -> vk::DescriptorSet
{
    return frames.currentDescriptorSet();
}

auto Renderer::currentFrameUBO() -> Buffer&
{
    return frames.frameUBO[frames.current()];
}

auto Renderer::currentNodeInstancesSSBO() -> Buffer&
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
