#include "RenderContext.hpp"
#include "Command.hpp"
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
      swapchain(
          device,
          physicalDevice,
          surface)
{
}

void RenderContext::recreateRenderTargets()
{
    // Ensure no work is using old swapchain-dependent resources
    device.handle.waitIdle();

    // Recreate swapchain first (defines extent + color format)
    swapchain.recreate(device, physicalDevice, surface);

    auto transientCommand = Command(device, vk::CommandPoolCreateFlagBits::eTransient);
    // Recreate depth target using new extent
    depth.recreate(
        device,
        allocator,
        transientCommand,
        swapchain.extent(),
        depth.format);
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
    return depth.format;
}
