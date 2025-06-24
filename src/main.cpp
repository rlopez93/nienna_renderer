
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include <volk.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION
#include <vma/vk_mem_alloc.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <VkBootstrap.h>

// Define these only in *one* .cc file.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include "tiny_gltf.h"

static SDL_Window *window = NULL;

int main(int argc, char *argv[])
{
    // --- SDL3 Init
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    // --- Create Window with Vulkan flag
    SDL_Window *window = SDL_CreateWindow("Vulkan + SDL3", 800, 600, SDL_WINDOW_VULKAN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return -1;
    }

    // --- Load Vulkan library
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        std::cerr << "SDL_Vulkan_LoadLibrary Error: " << SDL_GetError() << "\n";
        return -1;
    }

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

    Uint32             count_instance_extensions;
    const char *const *instance_extensions =
        SDL_Vulkan_GetInstanceExtensions(&count_instance_extensions);

    if (instance_extensions == NULL) {
        std::cerr << "SDL_Vulkan_GetInstanceExtensions failed:" << SDL_GetError()
                  << "\n";
        return -1;
    }

    int          count_extensions = count_instance_extensions + 1;
    const char **extensions =
        (const char **)SDL_malloc(count_extensions * sizeof(const char *));
    extensions[0] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    SDL_memcpy(&extensions[1], instance_extensions,
               count_instance_extensions * sizeof(const char *));

    // --- Create Vulkan instance using vk-bootstrap
    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("Vulkan SDL3 App")
                        .require_api_version(1, 4)
                        .use_default_debug_messenger()
                        .enable_validation_layers(true)
                        .enable_extensions(count_extensions, extensions)
                        .build();

    SDL_free(extensions);

    if (!inst_ret) {
        std::cerr << "vk-bootstrap instance creation failed: "
                  << inst_ret.error().message() << "\n";
        return -1;
    }

    vkb::Instance vkb_inst = inst_ret.value();
    VkInstance    instance = vkb_inst.instance;

    // --- Initialize volk
    if (volkInitialize() != VK_SUCCESS) {
        std::cerr << "Failed to initialize volk\n";
        return -1;
    }
    volkLoadInstance(instance);

    // --- Create Vulkan surface with SDL3
    VkSurfaceKHR surface;
    if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface)) {
        std::cerr << "SDL_Vulkan_CreateSurface failed\n";
        return -1;
    }

    // --- Pick physical device and create logical device using vk-bootstrap
    vkb::PhysicalDeviceSelector selector{vkb_inst};
    auto phys_ret = selector.set_surface(surface).require_present().select();

    if (!phys_ret) {
        std::cerr << "Failed to select physical device: " << phys_ret.error().message()
                  << "\n";
        return -1;
    }

    vkb::PhysicalDevice vkb_phys = phys_ret.value();

    vkb::DeviceBuilder dev_builder{vkb_phys};
    auto               dev_ret = dev_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create logical device: " << dev_ret.error().message()
                  << "\n";
        return -1;
    }

    vkb::Device      vkb_device = dev_ret.value();
    VkDevice         device     = vkb_device.device;
    VkPhysicalDevice gpu        = vkb_phys.physical_device;

    volkLoadDevice(device);

    VmaVulkanFunctions vulkanFunctions            = {};
    vulkanFunctions.vkGetInstanceProcAddr         = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr           = vkGetDeviceProcAddr;
    vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties =
        vkGetPhysicalDeviceMemoryProperties;
    vulkanFunctions.vkAllocateMemory                  = vkAllocateMemory;
    vulkanFunctions.vkFreeMemory                      = vkFreeMemory;
    vulkanFunctions.vkMapMemory                       = vkMapMemory;
    vulkanFunctions.vkUnmapMemory                     = vkUnmapMemory;
    vulkanFunctions.vkFlushMappedMemoryRanges         = vkFlushMappedMemoryRanges;
    vulkanFunctions.vkInvalidateMappedMemoryRanges    = vkInvalidateMappedMemoryRanges;
    vulkanFunctions.vkBindBufferMemory                = vkBindBufferMemory;
    vulkanFunctions.vkBindImageMemory                 = vkBindImageMemory;
    vulkanFunctions.vkGetBufferMemoryRequirements     = vkGetBufferMemoryRequirements;
    vulkanFunctions.vkGetImageMemoryRequirements      = vkGetImageMemoryRequirements;
    vulkanFunctions.vkCreateBuffer                    = vkCreateBuffer;
    vulkanFunctions.vkDestroyBuffer                   = vkDestroyBuffer;
    vulkanFunctions.vkCreateImage                     = vkCreateImage;
    vulkanFunctions.vkDestroyImage                    = vkDestroyImage;
    vulkanFunctions.vkCmdCopyBuffer                   = vkCmdCopyBuffer;
    vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
    vulkanFunctions.vkGetImageMemoryRequirements2KHR  = vkGetImageMemoryRequirements2;
    vulkanFunctions.vkBindBufferMemory2KHR            = vkBindBufferMemory2;
    vulkanFunctions.vkBindImageMemory2KHR             = vkBindImageMemory2;
    vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR =
        vkGetPhysicalDeviceMemoryProperties2;
    vulkanFunctions.vkGetDeviceBufferMemoryRequirements =
        vkGetDeviceBufferMemoryRequirements;
    vulkanFunctions.vkGetDeviceImageMemoryRequirements =
        vkGetDeviceImageMemoryRequirements;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice         = gpu;
    allocatorCreateInfo.device                 = device;
    allocatorCreateInfo.instance               = instance;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                              | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
        std::cerr << "Failed to create VMA allocator\n";
        return -1;
    }

    std::cout
        << "Vulkan + SDL3 + vk-bootstrap + Volk + VMA initialized successfully!\n";

    tinygltf::Model    model;
    tinygltf::TinyGLTF loader;
    std::string        err;
    std::string        warn;

    if (argc < 2) {
        std::cerr << "Ouchie, no .gltf file, oof owie\n";
        return EXIT_FAILURE;
    }

    namespace fs = std::filesystem;

    auto path = fs::path(argv[1]);

    bool ret = false;
    if (path.extension().string().ends_with("gltf")) {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, argv[1]);
    }

    else if (path.extension().string().ends_with("glb")) {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]);
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return EXIT_FAILURE;
    }
    // --- Main loop (minimal)
    bool      running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        SDL_Delay(16); // Simulate frame
    }

    // --- Cleanup
    vmaDestroyAllocator(allocator);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
