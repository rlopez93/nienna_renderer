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
#include "Command.hpp"
#include "Device.hpp"
#include "Frame.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Queue.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Timeline.hpp"
#include "Utility.hpp"
#include "gltfLoader.hpp"

struct Renderer {

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
    Timeline            timeline;

    Renderer();

    auto submit() -> void;
    auto present() -> void;
    auto render(Scene &scene) -> void;

    [[nodiscard]]
    auto getWindowExtent() const -> vk::Extent2D;
};
