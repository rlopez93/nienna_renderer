#include "Instance.hpp"

#include <SDL3/SDL_vulkan.h>

#include <vulkan/vulkan_to_string.hpp>

#include <fmt/base.h>

#include <iostream>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>

Instance::Instance()
    : context{},
      handle{createInstance(context)},
      debugUtils{createDebugUtilsMessenger(handle)}
{
}

// taken from Vulkan-Hpp
VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(
    vk::DebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
    vk::DebugUtilsMessageTypeFlagsEXT             messageTypes,
    vk::DebugUtilsMessengerCallbackDataEXT const *pCallbackData,
    void * /*pUserData*/)
{
    std::ostringstream message;

    message << vk::to_string(messageSeverity) << ": " << vk::to_string(messageTypes)
            << ":\n";
    message << std::string("\t") << "messageIDName   = <"
            << pCallbackData->pMessageIdName << ">\n";
    message << std::string("\t")
            << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
    message << std::string("\t") << "message         = <" << pCallbackData->pMessage
            << ">\n";
    if (0 < pCallbackData->queueLabelCount) {
        message << std::string("\t") << "Queue Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
            message << std::string("\t\t") << "labelName = <"
                    << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->cmdBufLabelCount) {
        message << std::string("\t") << "CommandBuffer Labels:\n";
        for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
            message << std::string("\t\t") << "labelName = <"
                    << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
        }
    }
    if (0 < pCallbackData->objectCount) {
        message << std::string("\t") << "Objects:\n";
        for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
            message << std::string("\t\t") << "Object " << i << "\n";
            message << std::string("\t\t\t") << "objectType   = "
                    << vk::to_string(pCallbackData->pObjects[i].objectType) << "\n";
            message << std::string("\t\t\t")
                    << "objectHandle = " << pCallbackData->pObjects[i].objectHandle
                    << "\n";
            if (pCallbackData->pObjects[i].pObjectName) {
                message << std::string("\t\t\t") << "objectName   = <"
                        << pCallbackData->pObjects[i].pObjectName << ">\n";
            }
        }
    }

#ifdef _WIN32
    MessageBox(NULL, message.str().c_str(), "Alert", MB_OK);
#else
    std::cout << message.str() << std::endl;
#endif

    return false;
}

auto Instance::createDebugUtilsMessenger(const vk::raii::Instance &instance)
    -> vk::raii::DebugUtilsMessengerEXT
{
    vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
        vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
        | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);

    vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
        vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
        | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
        | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

    vk::DebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfoEXT(
        {},
        severityFlags,
        messageTypeFlags,
        &debugMessageFunc);

    return {instance, debugUtilsMessengerCreateInfoEXT};
}

auto Instance::createInstance(const vk::raii::Context &context) -> vk::raii::Instance
{
    auto instanceLayers = std::vector{"VK_LAYER_KHRONOS_validation"};

    // Get available validation layers' names as vector<string>
    auto availableLayers =
        context.enumerateInstanceLayerProperties()
        | std::views::transform(
            [](const vk::LayerProperties &properties) -> std::string {
                return properties.layerName;
            })
        | std::ranges::to<std::vector>();

    // Drop validation layer if not present
    if (!std::ranges::contains(
            availableLayers,
            std::string{"VK_LAYER_KHRONOS_validation"})) {
        fmt::print(
            stderr,
            "Warning: Validation layer not found — continuing without it.\n");
        instanceLayers.clear();
    }

    auto instanceExtensions = std::vector{
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // debug messenger
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME,
        VK_KHR_SURFACE_EXTENSION_NAME // ALWAYS include explicitly
    };

    // Add SDL platform extensions
    Uint32 sdlExtensionCount     = 0;
    auto   sdlInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&sdlExtensionCount);
    if (!sdlInstanceExtensions) {
        fmt::print(
            stderr,
            "SDL_Vulkan_GetInstanceExtensions failed: {}\n",
            SDL_GetError());
        throw std::runtime_error("SDL Vulkan initialization failed.");
    }

    instanceExtensions.insert(
        instanceExtensions.end(),
        sdlInstanceExtensions,
        sdlInstanceExtensions + sdlExtensionCount);

    // Validate that all instance extensions are available
    auto availableExtensions =
        context.enumerateInstanceExtensionProperties()
        | std::views::transform(
            [](const vk::ExtensionProperties &properties) -> std::string {
                return properties.extensionName;
            })
        | std::ranges::to<std::vector>();

    for (auto extension : instanceExtensions) {
        if (!std::ranges::contains(availableExtensions, extension)) {
            fmt::print(
                stderr,
                "Required Vulkan instance extension missing: {}\n",
                extension);
            throw std::runtime_error("Missing required Vulkan instance extension.");
        }
    }

    const auto applicationInfo =
        vk::ApplicationInfo{"Nienna", 1, "Nienna", 1, vk::ApiVersion14};

    const auto instanceCreateInfo = vk::InstanceCreateInfo{
        {},
        &applicationInfo,
        instanceLayers,
        instanceExtensions};

    // construct vk::raii::Instance
    return {context, instanceCreateInfo};
}
