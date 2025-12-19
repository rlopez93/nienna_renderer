#include "Swapchain.hpp"

#include <algorithm>
#include <array>
#include <fmt/base.h>
#include <limits>
#include <stdexcept>

Swapchain::Swapchain(
    const Device         &device,
    const PhysicalDevice &physicalDevice,
    const Surface        &surface)
    : handle{nullptr}
{
    create(device, physicalDevice, surface);
}

// -----------------------------------------------------------------------------
// Surface format selection
// -----------------------------------------------------------------------------
auto Swapchain::chooseSurfaceFormat(
    const std::vector<vk::SurfaceFormatKHR> &availableFormats) -> vk::SurfaceFormatKHR
{
    constexpr auto preferredSurfaceFormats = std::array{
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Snorm};

    constexpr auto preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    for (auto preferredSurfaceFormat : preferredSurfaceFormats) {
        for (auto format : availableFormats) {
            if (format.format == preferredSurfaceFormat
                && format.colorSpace == preferredColorSpace) {
                return format;
            }
        }
    }

    return availableFormats.front();
}

// -----------------------------------------------------------------------------
// Present mode selection
// -----------------------------------------------------------------------------
auto Swapchain::choosePresentMode(
    const std::vector<vk::PresentModeKHR> &availablePresentModes) -> vk::PresentModeKHR
{
    constexpr auto preferredPresentModes =
        std::array{vk::PresentModeKHR::eMailbox, vk::PresentModeKHR::eImmediate};

    for (auto preferredPresentMode : preferredPresentModes) {
        for (auto mode : availablePresentModes) {
            if (mode == preferredPresentMode) {
                return mode;
            }
        }
    }

    return vk::PresentModeKHR::eFifo;
}

// -----------------------------------------------------------------------------
// Extent selection
// -----------------------------------------------------------------------------
auto Swapchain::chooseExtent(
    const vk::SurfaceCapabilitiesKHR &caps,
    const vk::Extent2D               &desired) -> vk::Extent2D
{
    if (caps.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return caps.currentExtent;
    }

    vk::Extent2D extent{};
    extent.width =
        std::clamp(desired.width, caps.minImageExtent.width, caps.maxImageExtent.width);
    extent.height = std::clamp(
        desired.height,
        caps.minImageExtent.height,
        caps.maxImageExtent.height);

    return extent;
}

// -----------------------------------------------------------------------------
// Swapchain creation helper
// -----------------------------------------------------------------------------
auto Swapchain::createSwapchain(
    const Device         &device,
    const PhysicalDevice &physicalDevice,
    const Surface        &surface,
    vk::SwapchainKHR      oldSwapchain) -> vk::raii::SwapchainKHR
{
    const auto capabilities =
        physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle);

    const auto formats = physicalDevice.handle.getSurfaceFormatsKHR(surface.handle);

    const auto presentModes =
        physicalDevice.handle.getSurfacePresentModesKHR(surface.handle);

    if (formats.empty() || presentModes.empty()) {
        throw std::runtime_error("Surface reports no formats or present modes");
    }

    const auto surfaceFormat = chooseSurfaceFormat(formats);
    const auto presentMode   = choosePresentMode(presentModes);
    const auto desiredExtent = getFramebufferExtent(device.window.get());
    const auto extent        = chooseExtent(capabilities, desiredExtent);

    uint32_t imageCount = 3u;
    imageCount          = std::max(imageCount, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0u) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    const auto graphicsFamilyQueueIndex = device.queueFamilyIndices.graphicsIndex;
    const auto presentFamilyQueueIndex  = device.queueFamilyIndices.presentIndex;

    const auto concurrent = (graphicsFamilyQueueIndex != presentFamilyQueueIndex);

    const auto queueFamilies =
        std::array{graphicsFamilyQueueIndex, presentFamilyQueueIndex};

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
        vk::True,
        oldSwapchain};

    return vk::raii::SwapchainKHR(device.handle, createInfo);
}

// -----------------------------------------------------------------------------
// Swapchain creation
// -----------------------------------------------------------------------------
auto Swapchain::create(
    const Device         &device,
    const PhysicalDevice &physicalDevice,
    const Surface        &surface,
    vk::SwapchainKHR      oldSwapchain) -> void
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

    // frame.recreate(device, images.size());
    imageInitialized.assign(images.size(), false);
}

// -----------------------------------------------------------------------------
// Swapchain recreation
// -----------------------------------------------------------------------------
auto Swapchain::recreate(
    const Device         &device,
    const PhysicalDevice &physicalDevice,
    const Surface        &surface) -> void
{
    device.graphicsQueue.waitIdle();
    // frame.index = 0;
    imageViews.clear();

    create(device, physicalDevice, surface, std::move(handle));

    needRecreate = false;
    // fmt::println(stderr, "Swapchain recreated!");
}

// -----------------------------------------------------------------------------
// Per-frame operations
// -----------------------------------------------------------------------------
auto Swapchain::acquireNextImage(vk::Semaphore signalSemaphore) -> vk::Result
{
    vk::Result result;
    std::tie(result, nextImageIndex) =
        handle.acquireNextImage(std::numeric_limits<uint64_t>::max(), signalSemaphore);
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
