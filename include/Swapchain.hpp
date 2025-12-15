#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"
#include "Frame.hpp"

struct Swapchain {
    explicit Swapchain(
        const Device         &device,
        const PhysicalDevice &physicalDevice,
        const Surface        &surface);

    auto create(
        const Device         &device,
        const PhysicalDevice &physicalDevice,
        const Surface        &surface,
        vk::SwapchainKHR      oldSwapchain = {}) -> void;

    auto recreate(
        const Device         &device,
        const PhysicalDevice &physicalDevice,
        const Surface        &surface) -> void;

    auto chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
        -> vk::SurfaceFormatKHR;

    auto choosePresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
        -> vk::PresentModeKHR;

    auto chooseExtent(
        const vk::SurfaceCapabilitiesKHR &caps,
        const vk::Extent2D               &desired) -> vk::Extent2D;

    auto createSwapchain(
        const Device         &device,
        const PhysicalDevice &physicalDevice,
        const Surface        &surface,
        vk::SwapchainKHR      oldSwapchain) -> vk::raii::SwapchainKHR;

    auto acquireNextImage() -> vk::Result;
    auto getNextImage() -> vk::Image;
    auto getNextImageView() -> vk::raii::ImageView &;
    auto getImageAvailableSemaphore() -> vk::Semaphore;
    auto getRenderFinishedSemaphore() -> vk::Semaphore;
    auto advance() -> void;

    vk::raii::SwapchainKHR           handle;
    std::vector<bool>                imageInitialized;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    Frame                            frame;
    bool                             needRecreate = false;
};
