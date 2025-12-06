#pragma once

#include <SDL3/SDL_video.h>

#include "vulkan_raii.hpp"

struct Instance {

    Instance();

    vk::raii::Context                context;
    vk::raii::Instance               handle;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
};
