#pragma once

#include <vector>
#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "DepthTarget.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"

// Forward declarations
// struct Window;
struct Allocator;
struct Command;

struct RenderContext {
    // Core Vulkan context
    Instance       instance;
    Surface        surface;
    PhysicalDevice physicalDevice;
    Device         device;
    Allocator      allocator;

    // Surface-dependent resources
    Swapchain   swapchain;
    DepthTarget depth;

    // Construction
    RenderContext(
        Window                          &window,
        const std::vector<const char *> &requiredExtensions);

    void recreateRenderTargets();

    [[nodiscard]]
    auto extent() const -> vk::Extent2D;

    [[nodiscard]]
    auto colorFormat() const -> vk::Format;

    [[nodiscard]]
    auto depthFormat() const -> vk::Format;
};
