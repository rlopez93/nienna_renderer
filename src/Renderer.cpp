#include "Renderer.hpp"

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
                  1}})}
{
}

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = swapchain.getRenderFinishedSemaphore();
    auto presentResult           = presentQueue.handle.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *swapchain.swapchain,
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
