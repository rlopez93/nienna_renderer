/*
 * This example code creates an SDL window and renderer, and then clears the
 * window to a different color every frame, so you'll effectively get a window
 * that's smoothly fading between colors.
 *
 * This code is public domain. Feel free to use it for any purpose!
 */
#include <SDL3/SDL_error.h>
#include <VkBootstrap.h>
#include <iostream>
#include <vulkan/vulkan_core.h>

#define SDL_MAIN_USE_CALLBACKS 1 /* use the callbacks instead of main() */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

#include <string>

/* We will use this renderer to draw into this window every frame. */
static SDL_Window   *window   = NULL;
static SDL_Renderer *renderer = NULL;

bool init_vulkan()
{
    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("Example Vulkan Application")
                        .request_validation_layers()
                        .use_default_debug_messenger()
                        .build();
    if (!inst_ret) {
        std::cerr << "Failed to create Vulkan instance. Error: "
                  << inst_ret.error().message() << "\n";
        return false;
    }
    vkb::Instance vkb_inst = inst_ret.value();

    window = SDL_CreateWindow("examples/renderer/clear", 640, 480, SDL_WINDOW_VULKAN);

    if (!window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    VkSurfaceKHR surface = VK_NULL_HANDLE;
    auto         sdl_success =
        SDL_Vulkan_CreateSurface(window, vkb_inst, VK_NULL_HANDLE, &surface);
    if (!sdl_success) {
        std::cerr << "Failed to select create window surface. Error: " << SDL_GetError()
                  << "\n";
        return false;
    }

    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto                        phys_ret = selector.set_surface(surface).select();
    if (!phys_ret) {
        std::cerr << "Failed to select Vulkan Physical Device. Error: "
                  << phys_ret.error().message() << "\n";
        return false;
    }

    vkb::DeviceBuilder device_builder{phys_ret.value()};
    // automatically propagate needed data from instance & physical device
    auto               dev_ret = device_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create Vulkan device. Error: "
                  << dev_ret.error().message() << "\n";
        return false;
    }
    vkb::Device vkb_device = dev_ret.value();

    // Get the VkDevice handle used in the rest of a vulkan application
    VkDevice device = vkb_device.device;

    // Get the graphics queue with a helper function
    auto graphics_queue_ret = vkb_device.get_queue(vkb::QueueType::graphics);
    if (!graphics_queue_ret) {
        std::cerr << "Failed to get graphics queue. Error: "
                  << graphics_queue_ret.error().message() << "\n";
        return false;
    }
    VkQueue graphics_queue = graphics_queue_ret.value();

    vkb::SwapchainBuilder swapchain_builder{vkb_device};
    auto                  swap_ret = swapchain_builder.build();
    if (!swap_ret) {
        std::cout << swap_ret.error().message() << "\n";
        return false;
    }
    vkb::Swapchain vkb_swapchain = swap_ret.value();

    // We did it!
    // Turned 400-500 lines of boilerplate into a fraction of that.

    // Time to cleanup
    vkb::destroy_swapchain(vkb_swapchain);
    vkb::destroy_device(vkb_device);
    vkb::destroy_surface(vkb_inst, surface);
    vkb::destroy_instance(vkb_inst);
    (void)device; // silence unused warning
    (void)graphics_queue;
    return true;
}

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    SDL_SetAppMetadata("Example Renderer Clear", "1.0", "com.example.renderer-clear");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    init_vulkan();

    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string        err;
    std::string        warn;

    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
    // bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary
    // glTF(.glb)

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS; /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    const double now =
        ((double)SDL_GetTicks()) / 1000.0; /* convert from milliseconds to seconds. */
    /* choose the color for the frame we will draw. The sine wave trick makes it fade
     * between colors smoothly. */
    const float red   = (float)(0.5 + 0.5 * SDL_sin(now));
    const float green = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 2 / 3));
    const float blue  = (float)(0.5 + 0.5 * SDL_sin(now + SDL_PI_D * 4 / 3));
    SDL_SetRenderDrawColorFloat(renderer, red, green, blue,
                                SDL_ALPHA_OPAQUE_FLOAT); /* new color, full alpha. */

    /* clear the window to the draw color. */
    SDL_RenderClear(renderer);

    /* put the newly-cleared rendering on the screen. */
    SDL_RenderPresent(renderer);

    return SDL_APP_CONTINUE; /* carry on with the program! */
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    /* SDL will clean up the window/renderer for us. */
}
