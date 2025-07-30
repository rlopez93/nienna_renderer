#pragma once

#include <SDL3/SDL_video.h>

#include <memory>

struct WindowDeleter {
    void operator()(SDL_Window *window) const;
};

using Window = std::unique_ptr<SDL_Window, WindowDeleter>;

auto createWindow(
    int width,
    int height) -> Window;

constexpr int windowWidth  = 800;
constexpr int windowHeight = 600;
