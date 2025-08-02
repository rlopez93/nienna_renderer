#pragma once

#include "vulkan_raii.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <VkBootstrap.h>

#include <fmt/base.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "Allocator.hpp"
#include "Device.hpp"
#include "Frame.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Utility.hpp"

struct Renderer {
    Renderer();

    [[nodiscard]]
    auto getWindowExtent() const -> vk::Extent2D;

    Window              window;
    Instance            instance;
    Surface             surface;
    PhysicalDevice      physicalDevice;
    Device              device;
    Queue               graphicsQueue;
    Queue               presentQueue;
    Swapchain           swapchain;
    Allocator           allocator;
    vk::Format          depthFormat;
    Image               depthImage;
    vk::raii::ImageView depthImageView;
    auto                present() -> void;
};
