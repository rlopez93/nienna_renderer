#include "RenderContext.hpp"

#include "Utility.hpp"

RenderContext::RenderContext(
    Window                          &window,
    const std::vector<const char *> &requiredExtensions)
    : instance{},
      surface{
          instance,
          window},
      physicalDevice{
          instance,
          requiredExtensions},
      device{
          physicalDevice,
          window,
          surface,
          requiredExtensions},
      allocator{
          instance,
          physicalDevice,
          device},
      swapchain{
          device,
          physicalDevice,
          surface},
      renderTargets{
          device,
          allocator.handle(),
          swapchain.extent(),
          swapchain.imageFormat,
          findDepthFormat(physicalDevice.handle),
      }
{
}

void RenderContext::recreateRenderTargets()
{
    device.handle.waitIdle();

    swapchain.recreate(device, physicalDevice, surface);

    renderTargets.recreate(
        device,
        swapchain.extent(),
        swapchain.imageFormat,
        renderTargets.mainDepth.format);
}

auto RenderContext::extent() const -> vk::Extent2D
{
    return swapchain.extent();
}

auto RenderContext::colorFormat() const -> vk::Format
{
    return swapchain.imageFormat;
}

auto RenderContext::depthFormat() const -> vk::Format
{
    return renderTargets.mainDepth.format;
}
