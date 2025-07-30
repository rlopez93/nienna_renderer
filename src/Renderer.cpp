#include "Renderer.hpp"

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = ctx.swapchain.getRenderFinishedSemaphore();
    auto presentResult           = ctx.present.handle.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *ctx.swapchain.swapchain,
            ctx.swapchain.nextImageIndex});

    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        ctx.swapchain.needRecreate = true;
    }

    else if (!(presentResult == vk::Result::eSuccess
               || presentResult == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }

    // advance to the next frame in the swapchain
    ctx.swapchain.advanceFrame();
}
auto RenderContext::getWindowExtent() const -> vk::Extent2D
{
    return physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle)
        .currentExtent;
}
RenderContext::RenderContext()
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
      present{
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
              1}
              .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment))},
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
    fmt::println(stderr, "{}x{}", getWindowExtent().width, getWindowExtent().height);
}
