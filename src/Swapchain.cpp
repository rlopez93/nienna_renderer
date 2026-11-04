#include "Swapchain.hpp"

#include <algorithm>
#include <array>
#include <fmt/base.h>
#include <limits>
#include <stdexcept>

Swapchain::Swapchain(
    Device const&         device,
    PhysicalDevice const& physicalDevice,
    Surface const&        surface)
{
    create(device, physicalDevice, surface);
}

auto Swapchain::chooseSurfaceFormat(
    std::vector<vk::SurfaceFormatKHR> const& availableFormats) -> vk::SurfaceFormatKHR
{
    constexpr auto preferredSurfaceFormats = std::array{
        vk::Format::eB8G8R8A8Srgb,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Unorm,
    };
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

auto Swapchain::choosePresentMode(
    std::vector<vk::PresentModeKHR> const& availablePresentModes) -> vk::PresentModeKHR
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

auto Swapchain::chooseExtent(
    vk::SurfaceCapabilitiesKHR const& capabilities,
    vk::Extent2D const&               desired) -> vk::Extent2D
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }
    vk::Extent2D extent{
        std::clamp(
            desired.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width),
        std::clamp(
            desired.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height)};

    return extent;
}

auto Swapchain::createSwapchain(
    Device const&         device,
    PhysicalDevice const& physicalDevice,
    Surface const&        surface,
    vk::SwapchainKHR      oldSwapchain) -> vk::raii::SwapchainKHR
{
    auto const capabilities =
        physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle);

    auto const formats = physicalDevice.handle.getSurfaceFormatsKHR(surface.handle);

    auto const presentModes =
        physicalDevice.handle.getSurfacePresentModesKHR(surface.handle);

    if (formats.empty() || presentModes.empty()) {
        throw std::runtime_error("Surface reports no formats or present modes");
    }

    auto const surfaceFormat = chooseSurfaceFormat(formats);
    auto const presentMode   = choosePresentMode(presentModes);
    auto const desiredExtent = getFramebufferExtent(device.window.get());
    swapchainExtent          = chooseExtent(capabilities, desiredExtent);

    if (swapchainExtent.width == 0 || swapchainExtent.height == 0) {
        throw std::runtime_error("Cannot create swapchain with zero extent");
    }

    uint32_t imageCount = std::clamp(
        3u,
        capabilities.minImageCount,
        capabilities.maxImageCount > 0 ? capabilities.maxImageCount : UINT32_MAX);

    auto const graphicsFamilyQueueIndex = device.queueFamilyIndices.graphicsIndex;
    auto const presentFamilyQueueIndex  = device.queueFamilyIndices.presentIndex;

    auto const concurrent = (graphicsFamilyQueueIndex != presentFamilyQueueIndex);

    auto const queueFamilies =
        std::array{graphicsFamilyQueueIndex, presentFamilyQueueIndex};

    vk::SwapchainCreateInfoKHR createInfo{
        {},
        surface.handle,
        imageCount,
        surfaceFormat.format,
        surfaceFormat.colorSpace,
        swapchainExtent,
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

auto Swapchain::create(
    Device const&         device,
    PhysicalDevice const& physicalDevice,
    Surface const&        surface,
    vk::SwapchainKHR      oldSwapchain) -> void
{
    handle = createSwapchain(device, physicalDevice, surface, oldSwapchain);

    images = handle.getImages();
    imageViews.clear();
    renderFinishedSemaphores.clear();

    auto const formats = physicalDevice.handle.getSurfaceFormatsKHR(surface.handle);
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

        renderFinishedSemaphores.emplace_back(device.handle, vk::SemaphoreCreateInfo{});
    }
}

auto Swapchain::recreate(
    Device const&         device,
    PhysicalDevice const& physicalDevice,
    Surface const&        surface) -> void
{
    device.graphicsQueue.waitIdle();
    imageViews.clear();

    create(device, physicalDevice, surface, std::move(handle));

    needRecreate = false;
}

auto Swapchain::acquireNextImage(vk::Semaphore signalSemaphore) -> vk::Result
{
    vk::Result result;
    std::tie(result, nextImageIndex) =
        handle.acquireNextImage(std::numeric_limits<uint64_t>::max(), signalSemaphore);
    return result;
}

auto Swapchain::renderFinishedSemaphore() const -> vk::Semaphore
{
    return *renderFinishedSemaphores[nextImageIndex];
}

auto Swapchain::nextImage() -> vk::Image
{
    return images[nextImageIndex];
}

auto Swapchain::nextImageView() -> vk::raii::ImageView&
{
    return imageViews[nextImageIndex];
}

auto Swapchain::extent() const -> vk::Extent2D
{
    return swapchainExtent;
}
