#pragma once

#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"

struct RenderContext {

    RenderContext(
        Window                          &window,
        const std::vector<const char *> &requiredExtensions);

    Instance       instance;
    Surface        surface;
    PhysicalDevice physicalDevice;
    Device         device;
};
