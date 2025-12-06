#include "Instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan_structs.hpp>

Instance::Instance()
    : context{},
      handle{
          context,
          vk::ApplicationInfo{
              "Nienna",
              1,
              {},
              {},
              VK_API_VERSION_1_4}},
      debugUtils{
          handle,
          vkbInstance.debug_messenger}
{
}

auto Instance::createVkbInstance() -> vkb::Instance
{
    std::vector<const char *> instanceExtensions{
        // TODO document what each extension enables
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
