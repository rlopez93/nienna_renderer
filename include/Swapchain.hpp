#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"

struct Swapchain {
    explicit Swapchain(
        Device const&         device,
        PhysicalDevice const& physicalDevice,
        Surface const&        surface);

    auto create(
        Device const&         device,
        PhysicalDevice const& physicalDevice,
        Surface const&        surface,
        vk::SwapchainKHR      oldSwapchain = {}) -> void;

    auto recreate(
        Device const&         device,
        PhysicalDevice const& physicalDevice,
        Surface const&        surface) -> void;

    [[nodiscard]]
    auto chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
        -> vk::SurfaceFormatKHR;

    [[nodiscard]]
    auto choosePresentMode(std::vector<vk::PresentModeKHR> const& availablePresentModes)
        -> vk::PresentModeKHR;

    [[nodiscard]]
    auto chooseExtent(
        vk::SurfaceCapabilitiesKHR const& caps,
        vk::Extent2D const&               desired) -> vk::Extent2D;

    [[nodiscard]]
    auto createSwapchain(
        Device const&         device,
        PhysicalDevice const& physicalDevice,
        Surface const&        surface,
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
    auto nextImageView() -> vk::raii::ImageView&;

    [[nodiscard]]
    auto extent() const -> vk::Extent2D;

    vk::raii::SwapchainKHR           handle = nullptr;
    vk::Extent2D                     swapchainExtent;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    bool                             needRecreate = false;

    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;
};
