#include "Instance.hpp"

#include <SDL3/SDL_vulkan.h>
#include <fmt/base.h>
#include <iostream>
#include <sstream>
#include <vulkan/vulkan_to_string.hpp>

Instance::Instance()
    : context{},
      handle{createInstance(context)},
      debugUtils{createDebugUtilsMessenger(handle)}
{
}

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
    auto applicationInfo =
        vk::ApplicationInfo{"Nienna", 1, "Nienna", 1, vk::ApiVersion14};
    // auto instanceLayerProperties = context.enumerateInstanceLayerProperties();
    auto instanceLayerNames = std::vector{"VK_LAYER_KHRONOS_validation"};

    // TODO: check layer

    auto instanceExtensions = std::vector<const char *>{
        // TODO: document what each extension enables
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME};

    {
        Uint32 sdlExtensionsCount;
        auto   sdlInstanceExtensions =
            SDL_Vulkan_GetInstanceExtensions(&sdlExtensionsCount);

        if (!sdlInstanceExtensions) {
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

    auto instanceCreateInfo = vk::InstanceCreateInfo{
        {},
        &applicationInfo,
        instanceLayerNames,
        instanceExtensions};

    return {context, instanceCreateInfo};
}
