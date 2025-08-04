#include "Swapchain.hpp"

#include <fmt/base.h>

Swapchain::Swapchain(Device &device)
    : handle(nullptr),
      frame(
          device,
          0u)
{
    create(device);
}

auto Swapchain::create(Device &device) -> void
{
    auto swapchainBuilder = vkb::SwapchainBuilder{device.vkbDevice};
    auto swapchainResult =
        swapchainBuilder
            //.set_old_swapchain(*swapchain)
            .set_image_usage_flags(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_R8G8B8A8_SRGB,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_B8G8R8A8_UNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_R8G8B8A8_SNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .set_desired_min_image_count(3)
            .build();

    if (!swapchainResult) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create swapchain: {}",
            swapchainResult.error().message());
        throw std::exception{};
    }

    handle = vk::raii::SwapchainKHR{device.handle, swapchainResult->swapchain};
    images = handle.getImages();
    imageViews.clear();
    imageFormat = vk::Format(swapchainResult->image_format);

    for (auto image : images) {
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
}

auto Swapchain::recreate(
    Device &device,
    Queue  &queue) -> void

{
    queue.handle.waitIdle();

    frame.index = 0;

    handle.clear();

    create(device);

    needRecreate = false;
}

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
