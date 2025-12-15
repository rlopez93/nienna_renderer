#include "Window.hpp"

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>
#include <fmt/format.h>

void WindowDeleter::operator()(SDL_Window *window) const
{
    SDL_DestroyWindow(window);
}

auto createWindow(
    int width,
    int height) -> Window
{

#define SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS 1

    // init SDL3
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        throw std::runtime_error{fmt::format("SDL_Init Error: {}\n", SDL_GetError())};
    }

    // Dynamically load Vulkan loader library
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        throw std::runtime_error{
            fmt::format("SDL_Vulkan_LoadLibrary Error: {}\n", SDL_GetError())};
    }

    // create Vulkan window
    auto window = SDL_CreateWindow(
        "Vulkan + SDL3",
        800,
        600,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        throw std::runtime_error{
            fmt::format("SDL_CreateWindow Error: {}\n", SDL_GetError())};
    }

    return Window{window};
}

auto getFramebufferExtent(SDL_Window *window) -> vk::Extent2D
{
    int width  = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window, &width, &height);

    return vk::Extent2D{static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
}
