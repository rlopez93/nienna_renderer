#include "Renderer.hpp"
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

    fmt::print(stderr, "\n\nBefore vkQueueSubmit2()\n\n");
    graphicsQueue.handle.submit2(
        vk::SubmitInfo2{
            {},
            waitSemaphoreSubmitInfo,
            commandBufferSubmitInfo,
            signalSemaphoreSubmitInfos});
    fmt::print(stderr, "\n\nAfter vkQueueSubmit2()\n\n");
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

    // advance to the next frame in the swapchain
    swapchain.advanceFrame();
}

auto Renderer::getWindowExtent() const -> vk::Extent2D
{
    return physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle)
        .currentExtent;
}
