#pragma once

#include "vulkan_raii.hpp"

#include "Instance.hpp"
#include "Window.hpp"

auto createSurface(
    Instance &instance,
    Window   &window) -> vk::raii::SurfaceKHR;

struct Surface {
    Surface(
        Instance &instance,
        Window   &window);
    vk::raii::SurfaceKHR handle;
};
