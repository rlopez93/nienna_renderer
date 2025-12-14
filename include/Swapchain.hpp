#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"
#include "Frame.hpp"

struct Swapchain {
    explicit Swapchain(
        Device         &device,
        PhysicalDevice &physicalDevice,
        Surface        &surface);

    auto create(
        Device         &device,
        PhysicalDevice &physicalDevice,
        Surface        &surface) -> void;

    auto recreate(
        Device         &device,
        PhysicalDevice &physicalDevice,
        Surface        &surface)

        -> void;

    auto acquireNextImage() -> vk::Result;

    auto getNextImage() -> vk::Image;

    auto getNextImageView() -> vk::raii::ImageView &;

    auto getImageAvailableSemaphore() -> vk::Semaphore;

    auto getRenderFinishedSemaphore() -> vk::Semaphore;

    auto advance() -> void;

    vk::raii::SwapchainKHR           handle;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    Frame                            frame;
    bool                             needRecreate = false;
};
