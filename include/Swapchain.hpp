#pragma once

#include <vector>

#include "vulkan_raii.hpp"

#include "Device.hpp"
#include "Frame.hpp"
#include "Queue.hpp"

struct Swapchain {
    vk::raii::SwapchainKHR           swapchain;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    Frame                            frame;
    bool                             needRecreate = false;

    Swapchain(Device &device);

    auto create(Device &device) -> void;

    auto recreate(
        Device &device,
        Queue  &queue)

        -> void;

    auto acquireNextImage() -> vk::Result;

    auto getNextImage() -> vk::Image;

    auto getNextImageView() -> vk::ImageView;

    auto getImageAvailableSemaphore() -> vk::Semaphore;

    auto getRenderFinishedSemaphore() -> vk::Semaphore;

    auto advanceFrame() -> void;
};
