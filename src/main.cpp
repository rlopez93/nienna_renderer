#include <volk.h>

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

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

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

static SDL_Window *window = NULL;

int main(int argc, char *argv[])
{
    if (argc < 2) {
        std::cerr << "Oof ouchie, need a gltf file, ouchie owie\n";
        return EXIT_FAILURE;
    }

    auto path = std::filesystem::path(argv[1]);

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(path);
    if (!bool(gltfFile)) {
        std::cerr << "Failed to open glTF file: "
                  << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        return EXIT_FAILURE;
    }

    static constexpr auto supportedExtensions =
        fastgltf::Extensions::KHR_mesh_quantization
        | fastgltf::Extensions::KHR_texture_transform
        | fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser(supportedExtensions);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages
        | fastgltf::Options::GenerateMeshIndices;

    auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

    if (fastgltf::getErrorName(asset.error()) != "None") {
        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error())
                  << '\n';
        return EXIT_FAILURE;
    }

    for (auto &mesh : asset->meshes) {
        for (auto it = mesh.primitives.begin(); it != mesh.primitives.end(); ++it) {
            auto *positionIt = it->findAttribute("POSITION");
        }
    }

    // viewer->asset = std::move(asset.get());

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
