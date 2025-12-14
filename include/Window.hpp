#pragma once

#include <vulkan/vulkan.hpp>

#include <SDL3/SDL_video.h>

#include <memory>

constexpr int windowWidth  = 800;
constexpr int windowHeight = 600;

struct WindowDeleter {
    void operator()(SDL_Window *window) const;
};

using Window = std::unique_ptr<SDL_Window, WindowDeleter>;

auto createWindow(
    int width,
    int height) -> Window;

auto getFramebufferExtent(SDL_Window *window) -> vk::Extent2D;
