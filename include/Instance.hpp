#pragma once

#include <SDL3/SDL_video.h>

#include "vulkan_raii.hpp"

#include <VkBootstrap.h>

auto createVkbInstance() -> vkb::Instance;

struct Instance {
    vk::raii::Context                context;
    vkb::Instance                    vkbInstance;
    vk::raii::Instance               handle;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
    Instance();
};
