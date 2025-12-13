#include "Renderer.hpp"
#include "App.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"
#include <ranges>

Renderer::Renderer()
    : window{createWindow(
          windowWidth,
          windowHeight)},
      instance{},
      surface{
          instance,
          window},
      requiredExtensions{
          {VK_KHR_SWAPCHAIN_EXTENSION_NAME},
          {VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME},
          {VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME},
          {VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME},
          {VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME},
          {VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME},
          {VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME},
          {VK_EXT_ROBUSTNESS_2_EXTENSION_NAME},
          {VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME},
          {VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME},
          {VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME},
          {VK_EXT_MEMORY_BUDGET_EXTENSION_NAME},

      },
      physicalDevice{
          instance,
          requiredExtensions},
      device{
          physicalDevice,
          surface,
          requiredExtensions},
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
          swapchain.frame.maxFramesInFlight},

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
        .QueueFamily         = graphicsQueue.index,
        .Queue               = *graphicsQueue.handle,
        .DescriptorPool      = *imguiDescriptorPool,
        .MinImageCount       = 3,
        .ImageCount          = 3,
        .MSAASamples         = VK_SAMPLE_COUNT_1_BIT,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{{}, swapchain.imageFormat}};

    ImGui_ImplVulkan_Init(&init_info);
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

    // FIXME:
    // Swapchain recreation logic

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

Renderer::~Renderer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
