#include "Renderer.hpp"
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/base.h>

#include <VkBootstrap.h>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

auto RenderContext::init() -> RenderContext
{
    // init vk::raii::Context necessary for vulkan.hpp RAII functionality
    auto context = vk::raii::Context{};

    // init SDL3 and create window
    auto window = [&] -> RenderContext::WindowPtr {
        // init SDL3
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            fmt::print(stderr, "SDL_Init Error: {}\n", SDL_GetError());
            return {};
        }

        // Dynamically load Vulkan loader library
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            fmt::print(stderr, "SDL_Vulkan_LoadLibrary Error: {}\n", SDL_GetError());
            return {};
        }

        // create Vulkan window
        auto window = SDL_CreateWindow("Vulkan + SDL3", 800, 600, SDL_WINDOW_VULKAN);
        if (!window) {
            fmt::print(stderr, "SDL_CreateWindow Error: {}\n", SDL_GetError());
            return {};
        }

        return RenderContext::WindowPtr{window};
    }();

    // if (!window) {
    //     return EXIT_FAILURE;
    // }
    //
#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

    std::vector<const char *> instance_extensions{
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME};

    {
        Uint32             sdl_extensions_count;
        const char *const *sdl_instance_extensions =
            SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

        if (sdl_instance_extensions == nullptr) {
            fmt::print(
                stderr,
                "SDL_Vulkan_GetInstanceExtensions Error: {}\n",
                SDL_GetError());
            throw std::exception{};
        }
        instance_extensions.insert(
            instance_extensions.end(),
            sdl_instance_extensions,
            sdl_instance_extensions + sdl_extensions_count);
    }

    // --- Create Vulkan instance using vk-bootstrap
    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("Vulkan SDL3 App")
                        .require_api_version(1, 4)
                        .use_default_debug_messenger()
                        .enable_validation_layers(true)
                        .enable_extensions(instance_extensions)
                        .build();

    if (!inst_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap instance creation failed: {}\n",
            inst_ret.error().message());
        throw std::exception{};
    }

    auto vkb_inst = vkb::Instance{inst_ret.value()};
    auto instance = vk::raii::Instance{context, vkb_inst.instance};
    auto debugUtils =
        vk::raii::DebugUtilsMessengerEXT{instance, vkb_inst.debug_messenger};

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    auto surface = [&] -> vk::raii::SurfaceKHR {
        VkSurfaceKHR surface_vk;
        if (!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &surface_vk)) {
            fmt::print(stderr, "SDL_Vulkan_CreateSurface failed: {}\n", SDL_GetError());
            return nullptr;
        }
        return {instance, surface_vk};
    }();

    // --- Pick physical device and create logical device using vk-bootstrap
    vkb::PhysicalDeviceSelector selector{vkb_inst};

    auto features = instance.enumeratePhysicalDevices()
                        .front()
                        .getFeatures2<
                            vk::PhysicalDeviceFeatures2,
                            vk::PhysicalDeviceVulkan11Features,
                            vk::PhysicalDeviceVulkan12Features,
                            vk::PhysicalDeviceVulkan13Features,
                            vk::PhysicalDeviceVulkan14Features,
                            vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT,
                            vk::PhysicalDeviceExtendedDynamicState2FeaturesEXT,
                            vk::PhysicalDeviceExtendedDynamicState3FeaturesEXT,
                            vk::PhysicalDeviceSwapchainMaintenance1FeaturesEXT>();

    auto phys_ret =
        selector.set_surface(*surface)
            .set_required_features(features.get<vk::PhysicalDeviceFeatures2>().features)
            .require_present()
            .select();

    if (!phys_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to select physical device: {}\n",
            phys_ret.error().message());
        throw std::exception{};
    }

    auto vkb_phys = phys_ret.value();
    // auto deviceExtensions = vk::getDeviceExtensions();
    //
    // fmt::print(stderr, "device extensions: {}\n", deviceExtensions.size());
    //
    // for (const auto &extension : deviceExtensions) {
    //     std::vector<const char *> enabledExtensions;
    //     enabledExtensions.push_back(extension.c_str());
    //     bool enabled = vkb_phys.enable_extensions_if_present(enabledExtensions);
    //
    //     fmt::print(stderr, "<{}> enabled: {}\n", extension, enabled);
    // }

    auto physicalDevice = vk::raii::PhysicalDevice(instance, vkb_phys.physical_device);

    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceFeatures2>());
    vkb_phys.enable_extension_features_if_present(
        features.get<vk::PhysicalDeviceVulkan11Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan12Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan13Features>());
    // vkb_phys.enable_extension_features_if_present(
    //     features.get<vk::PhysicalDeviceVulkan14Features>());

    auto dev_builder = vkb::DeviceBuilder{vkb_phys};
    auto dev_ret     = dev_builder.build();
    if (!dev_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create logical device: {}\n",
            dev_ret.error().message());
        throw std::exception{};
    }

    auto vkb_device = dev_ret.value();
    auto device     = vk::raii::Device{physicalDevice, vkb_device.device};

    auto vkb_graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueue     = vk::raii::Queue{device, vkb_graphicsQueue};
    auto graphicsQueueIndex =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    auto vkb_presentQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto presentQueue     = vk::raii::Queue{device, vkb_presentQueue};
    auto presentQueueIndex =
        vkb_device.get_queue_index(vkb::QueueType::present).value();

    auto swap_builder = vkb::SwapchainBuilder{vkb_device};
    auto swap_ret =
        swap_builder
            .set_image_usage_flags(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_R8G8B8A8_SRGB,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_B8G8R8A8_UNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_R8G8B8A8_SNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .set_desired_min_image_count(3)
            .build();

    if (!swap_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create swapchain: {}",
            swap_ret.error().message());
        throw std::exception{};
    }

    auto vkb_swapchain = swap_ret.value();
    auto swapchain     = vk::raii::SwapchainKHR(device, vkb_swapchain.swapchain);
    auto imageFormat   = vk::Format(vkb_swapchain.image_format);
    auto depthFormat   = findDepthFormat(physicalDevice);
    auto windowExtent = physicalDevice.getSurfaceCapabilitiesKHR(surface).currentExtent;

    return RenderContext{
        std::move(context),
        std::move(instance),
        std::move(debugUtils),
        std::move(window),
        std::move(surface),
        std::move(physicalDevice),
        std::move(device),
        std::move(graphicsQueue),
        std::move(presentQueue),
        graphicsQueueIndex,
        presentQueueIndex,
        std::move(swapchain),
        imageFormat,
        depthFormat,
        windowExtent};
}

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format
{
    auto candidateFormats = std::array{

        // vk::Format::eD32Sfloat,
        // vk::Format::eD32SfloatS8Uint,
        // vk::Format::eD24UnormS8Uint,
        vk::Format::eD16Unorm,        // depth only
        vk::Format::eD32Sfloat,       // depth only
        vk::Format::eD32SfloatS8Uint, // depth-stencil
        vk::Format::eD24UnormS8Uint   // depth-stencil
    };

    for (auto format : candidateFormats) {
        auto properties = physicalDevice.getFormatProperties(format);

        if ((properties.optimalTilingFeatures
             & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            return format;
        }
    }

    assert(false);

    return vk::Format::eUndefined;
}
