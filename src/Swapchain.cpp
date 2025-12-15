#include "Swapchain.hpp"

#include <algorithm>
#include <array>
#include <fmt/base.h>
#include <limits>
#include <stdexcept>

Swapchain::Swapchain(
    Device         &device,
    PhysicalDevice &physicalDevice,
    Surface        &surface)
    : handle{nullptr},
      frame(
          device,
          0u)
{
    create(device, physicalDevice, surface);
}

namespace
{

// -----------------------------------------------------------------------------
// Surface format selection
// -----------------------------------------------------------------------------
vk::SurfaceFormatKHR
chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &formats)
{
    for (const auto &f : formats) {
        if (f.format == vk::Format::eR8G8B8A8Srgb
            && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return f;
        }
    }

    for (const auto &f : formats) {
        if (f.format == vk::Format::eB8G8R8A8Unorm
            && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return f;
        }
    }

    for (const auto &f : formats) {
        if (f.format == vk::Format::eR8G8B8A8Snorm
            && f.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return f;
        }
    }

    return formats.front();
}

// -----------------------------------------------------------------------------
// Present mode selection
// -----------------------------------------------------------------------------
vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR> &modes)
{
    for (auto m : modes) {
        if (m == vk::PresentModeKHR::eMailbox) {
            return m;
        }
    }

    for (auto m : modes) {
        if (m == vk::PresentModeKHR::eImmediate) {
            return m;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

// -----------------------------------------------------------------------------
// Extent selection
// -----------------------------------------------------------------------------
vk::Extent2D chooseExtent(
    const vk::SurfaceCapabilitiesKHR &caps,
    const vk::Extent2D               &desired)
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }

    vk::Extent2D extent = desired;
    extent.width =
        std::clamp(extent.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(
        extent.height,
        caps.minImageExtent.height,
        caps.maxImageExtent.height);

    return extent;
}

// -----------------------------------------------------------------------------
// Swapchain creation helper
// -----------------------------------------------------------------------------
vk::raii::SwapchainKHR createSwapchain(
    Device          &device,
    PhysicalDevice  &physicalDevice,
    Surface         &surface,
    vk::SwapchainKHR oldSwapchain)
{
    const auto capabilities =
        physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle);

    const auto formats = physicalDevice.handle.getSurfaceFormatsKHR(surface.handle);

    const auto presentModes =
        physicalDevice.handle.getSurfacePresentModesKHR(surface.handle);

    if (formats.empty() || presentModes.empty()) {
        throw std::runtime_error("Surface reports no formats or present modes");
    }

    const vk::SurfaceFormatKHR surfaceFormat = chooseSurfaceFormat(formats);

    const vk::PresentModeKHR presentMode = choosePresentMode(presentModes);

    const vk::Extent2D desiredExtent = getFramebufferExtent(device.window.get());

    fmt::print("(desiredExtent: [{}, {}]", desiredExtent.width, desiredExtent.height);

    const vk::Extent2D extent = chooseExtent(capabilities, desiredExtent);

    fmt::print("(extent: [{}, {}]", extent.width, extent.height);

    uint32_t imageCount = 3u;
    imageCount          = std::max(imageCount, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    const uint32_t graphicsFamily = device.queueFamilyIndices.graphicsIndex;
    const uint32_t presentFamily  = device.queueFamilyIndices.presentIndex;

    const bool concurrent = graphicsFamily != presentFamily;

    const std::array<uint32_t, 2> queueFamilies{graphicsFamily, presentFamily};

    vk::SwapchainCreateInfoKHR createInfo{
        {},
        surface.handle,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        extent,
        1u,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        concurrent ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive,
        concurrent ? static_cast<uint32_t>(queueFamilies.size()) : 0u,
        concurrent ? queueFamilies.data() : nullptr,
        capabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        oldSwapchain};

    return vk::raii::SwapchainKHR(device.handle, createInfo);
}

} // namespace

// -----------------------------------------------------------------------------
// Swapchain creation
// -----------------------------------------------------------------------------
auto Swapchain::create(
    Device            &device,
    PhysicalDevice    &physicalDevice,
    Surface           &surface,
    vk::SwapchainKHR &&oldSwapchain) -> void
{
    handle = createSwapchain(device, physicalDevice, surface, oldSwapchain);

    images = handle.getImages();
    imageViews.clear();

    const auto formats = physicalDevice.handle.getSurfaceFormatsKHR(surface.handle);
    imageFormat        = chooseSurfaceFormat(formats).format;

    for (vk::Image image : images) {
        imageViews.emplace_back(
            device.handle,
            vk::ImageViewCreateInfo{
                {},
                image,
                vk::ImageViewType::e2D,
                imageFormat,
                {},
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
    }

    frame.recreate(device, images.size());
    imageInitialized.assign(images.size(), false);
}

// -----------------------------------------------------------------------------
// Swapchain recreation
// -----------------------------------------------------------------------------
auto Swapchain::recreate(
    Device         &device,
    PhysicalDevice &physicalDevice,
    Surface        &surface) -> void
{
    device.graphicsQueue.waitIdle();

    frame.index = 0;

    imageViews.clear();

    create(device, physicalDevice, surface, std::move(handle));

    needRecreate = false;

    fmt::println(stderr, "Swapchain recreated!");
}

// -----------------------------------------------------------------------------
// Per-frame operations
// -----------------------------------------------------------------------------
auto Swapchain::acquireNextImage() -> vk::Result
{
    vk::Result result;
    std::tie(result, nextImageIndex) = handle.acquireNextImage(
        std::numeric_limits<uint64_t>::max(),
        getImageAvailableSemaphore());
    return result;
}

auto Swapchain::getNextImage() -> vk::Image
{
    return images[nextImageIndex];
}

auto Swapchain::getNextImageView() -> vk::raii::ImageView &
{
    return imageViews[nextImageIndex];
}

auto Swapchain::getImageAvailableSemaphore() -> vk::Semaphore
{
    return frame.imageAvailableSemaphores[frame.index];
}

auto Swapchain::getRenderFinishedSemaphore() -> vk::Semaphore
{
    return frame.renderFinishedSemaphores[nextImageIndex];
}

auto Swapchain::advance() -> void
{
    frame.index = (frame.index + 1) % frame.maxFramesInFlight;
}
