#pragma once

#include "vulkan_raii.hpp"

#include "Instance.hpp"
#include "Window.hpp"

struct Surface {

    Surface(
        Instance &instance,
        Window   &window);

    static auto createSurface(
        Instance &instance,
        Window   &window) -> vk::raii::SurfaceKHR;

    vk::raii::SurfaceKHR handle;
};
