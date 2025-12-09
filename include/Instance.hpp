#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_raii.hpp>

struct Instance {

    Instance();

    static auto createInstance(const vk::raii::Context &context) -> vk::raii::Instance;
    static auto createDebugUtilsMessenger(const vk::raii::Instance &instance)
        -> vk::raii::DebugUtilsMessengerEXT;

    vk::raii::Context                context;
    vk::raii::Instance               handle;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
};
