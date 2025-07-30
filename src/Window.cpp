#include "Window.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>

void WindowDeleter::operator()(SDL_Window *window) const
{
    SDL_DestroyWindow(window);
}

auto createWindow(
    int width,
    int height) -> Window
{

    // init SDL3
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fmt::print(stderr, "SDL_Init Error: {}\n", SDL_GetError());
        return nullptr;
    }

    // Dynamically load Vulkan loader library
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        fmt::print(stderr, "SDL_Vulkan_LoadLibrary Error: {}\n", SDL_GetError());
        return nullptr;
    }

    // create Vulkan window
    auto window = SDL_CreateWindow("Vulkan + SDL3", 800, 600, SDL_WINDOW_VULKAN);
    if (!window) {
        fmt::print(stderr, "SDL_CreateWindow Error: {}\n", SDL_GetError());
        return nullptr;
    }

    return Window{window};
}
