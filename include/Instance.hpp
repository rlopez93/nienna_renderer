#pragma once

#include <vulkan/vulkan_raii.hpp>

struct Instance {

    Instance();

    static auto createInstance(vk::raii::Context const& context) -> vk::raii::Instance;
    static auto createDebugUtilsMessenger(vk::raii::Instance const& instance)
        -> vk::raii::DebugUtilsMessengerEXT;

    vk::raii::Context                context;
    vk::raii::Instance               handle;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
};
