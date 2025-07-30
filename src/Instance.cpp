#define VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
#include "Instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>

auto createVkbInstance() -> vkb::Instance
{
    std::vector<const char *> instanceExtensions{
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME};

    {
        Uint32             sdlExtensionsCount;
        const char *const *sdlInstanceExtensions =
            SDL_Vulkan_GetInstanceExtensions(&sdlExtensionsCount);

        if (sdlInstanceExtensions == nullptr) {
            fmt::print(
                stderr,
                "SDL_Vulkan_GetInstanceExtensions Error: {}\n",
                SDL_GetError());
            throw std::exception{};
        }

        instanceExtensions.insert(
            instanceExtensions.end(),
            sdlInstanceExtensions,
            sdlInstanceExtensions + sdlExtensionsCount);
    }

    // --- Create Vulkan instance using vk-bootstrap
    vkb::InstanceBuilder instanceBuilder;
    auto instanceResult = instanceBuilder.set_app_name("Vulkan SDL3 App")
                              .require_api_version(1, 4)
                              .use_default_debug_messenger()
                              .enable_validation_layers(true)
                              .enable_extensions(instanceExtensions)
                              .build();

    if (!instanceResult) {
        fmt::print(
            stderr,
            "vk-bootstrap instance creation failed: {}\n",
            instanceResult.error().message());
        throw std::exception{};
    }

    return instanceResult.value();
}
Instance::Instance()
    : context{},
      vkbInstance{createVkbInstance()},
      handle{
          context,
          vkbInstance.instance},
      debugUtils{
          handle,
          vkbInstance.debug_messenger}
{
    VULKAN_HPP_DEFAULT_DISPATCHER.init();
}
