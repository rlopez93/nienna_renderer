#pragma once

#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Allocator.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "RenderTargets.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"

struct RenderContext {
    Instance       instance;
    Surface        surface;
    PhysicalDevice physicalDevice;
    Device         device;
    Allocator      allocator;

    Swapchain     swapchain;
    RenderTargets renderTargets;

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
