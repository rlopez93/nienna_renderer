#include "Surface.hpp"

#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>

auto createSurface(
    Instance &instance,
    Window   &window) -> vk::raii::SurfaceKHR
{
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window.get(), *instance.handle, nullptr, &surface)) {
        fmt::print(stderr, "SDL_Vulkan_CreateSurface failed: {}\n", SDL_GetError());
        return nullptr;
    }
    return {instance.handle, surface};
}

Surface::Surface(
    Instance &instance,
    Window   &window)
    : handle{createSurface(
          instance,
          window)}
{
}
