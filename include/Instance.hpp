#pragma once

#include <SDL3/SDL_video.h>

#include "vulkan_raii.hpp"

#include <VkBootstrap.h>

struct Instance {

    Instance();

    static auto createVkbInstance() -> vkb::Instance;

    vk::raii::Context                context;
    vkb::Instance                    vkbInstance;
    vk::raii::Instance               handle;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
};
