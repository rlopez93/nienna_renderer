#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"

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

    [[nodiscard]]
    auto chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
        -> vk::SurfaceFormatKHR;

    [[nodiscard]]
    auto choosePresentMode(const std::vector<vk::PresentModeKHR> &availablePresentModes)
        -> vk::PresentModeKHR;

    [[nodiscard]]
    auto chooseExtent(
        const vk::SurfaceCapabilitiesKHR &caps,
        const vk::Extent2D               &desired) -> vk::Extent2D;

    [[nodiscard]]
    auto createSwapchain(
        const Device         &device,
        const PhysicalDevice &physicalDevice,
        const Surface        &surface,
        vk::SwapchainKHR      oldSwapchain) -> vk::raii::SwapchainKHR;

    [[nodiscard]]
    auto acquireNextImage(vk::Semaphore signalSemaphore) -> vk::Result;

    [[nodiscard]]
    auto imageAvailableSemaphore() const -> vk::Semaphore;
    [[nodiscard]]
    auto renderFinishedSemaphore() const -> vk::Semaphore;

    [[nodiscard]]
    auto nextImage() -> vk::Image;

    [[nodiscard]]
    auto nextImageView() -> vk::raii::ImageView &;

    [[nodiscard]]
    auto extent() const -> vk::Extent2D;

    vk::raii::SwapchainKHR           handle = nullptr;
    vk::Extent2D                     swapchainExtent;
    std::vector<bool>                imageInitialized;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    bool                             needRecreate = false;

    // Per-frame synchronization
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
};
