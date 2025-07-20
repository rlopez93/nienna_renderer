#pragma once
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <memory>
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <utility>
#include <vulkan/vulkan_raii.hpp>

#include <VkBootstrap.h>

struct Renderer;

struct RenderContext {
    friend class Renderer;

    static constexpr auto SDL_WindowDeleter = [](SDL_Window *window) {
        SDL_DestroyWindow(window);
    };

    using WindowPtr = std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>;

    RenderContext(
        vkb::InstanceBuilder             instanceBuilder,
        vkb::Instance                    vkbInstance,
        vkb::PhysicalDeviceSelector      physicalDeviceSelector,
        vkb::PhysicalDevice              vkbPhysicalDevice,
        vkb::DeviceBuilder               deviceBuilder,
        vkb::Device                      vkbDevice,
        vkb::SwapchainBuilder            swapchainBuilder,
        vkb::Swapchain                   vkbSwapchain,
        vk::raii::Context                context,
        vk::raii::Instance               instance,
        vk::raii::DebugUtilsMessengerEXT debugUtils,
        WindowPtr                        window,
        vk::raii::SurfaceKHR             surface,
        vk::raii::PhysicalDevice         physicalDevice,
        vk::raii::Device                 device,
        vk::raii::Queue                  graphicsQueue,
        vk::raii::Queue                  presentQueue,
        uint32_t                         graphicsQueueIndex,
        uint32_t                         presentQueueIndex,
        vk::raii::SwapchainKHR           swapchain,
        vk::Format                       imageFormat,
        vk::Format                       depthFormat,
        vk::Extent2D                     windowExtent)
        : instanceBuilder{std::move(instanceBuilder)},
          vkbInstance{vkbInstance},
          vkbPhysicalDevice{std::move(vkbPhysicalDevice)},
          physicalDeviceSelector{std::move(physicalDeviceSelector)},
          deviceBuilder{std::move(deviceBuilder)},
          vkbDevice{std::move(vkbDevice)},
          swapchainBuilder{std::move(swapchainBuilder)},
          vkbSwapchain{vkbSwapchain},

          context{std::move(context)},
          instance{std::move(instance)},
          debugUtils{std::move(debugUtils)},
          window{std::move(window)},
          surface{std::move(surface)},
          physicalDevice{std::move(physicalDevice)},
          device{std::move(device)},
          graphicsQueue{std::move(graphicsQueue)},
          presentQueue{std::move(presentQueue)},
          graphicsQueueIndex{graphicsQueueIndex},
          presentQueueIndex{presentQueueIndex},
          swapchain{std::move(swapchain)},
          imageFormat{imageFormat},
          depthFormat{depthFormat},
          windowExtent{windowExtent}
    {
    }

    ~RenderContext()
    {
        SDL_Quit();
    }

  private:
    static auto init() -> RenderContext;

    vkb::InstanceBuilder        instanceBuilder;
    vkb::Instance               vkbInstance;
    vkb::PhysicalDeviceSelector physicalDeviceSelector;
    vkb::PhysicalDevice         vkbPhysicalDevice;
    vkb::DeviceBuilder          deviceBuilder;
    vkb::Device                 vkbDevice;
    vkb::SwapchainBuilder       swapchainBuilder;
    vkb::Swapchain              vkbSwapchain;

  public:
    auto                             createSwapchain();
    vk::raii::Context                context;
    vk::raii::Instance               instance;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
    WindowPtr                        window;
    vk::raii::SurfaceKHR             surface;
    vk::raii::PhysicalDevice         physicalDevice;
    vk::raii::Device                 device;
    vk::raii::Queue                  graphicsQueue;
    vk::raii::Queue                  presentQueue;
    uint32_t                         graphicsQueueIndex;
    uint32_t                         presentQueueIndex;
    vk::raii::SwapchainKHR           swapchain;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    vk::Extent2D                     windowExtent;
};

struct Renderer {
    friend struct RenderContext;
    Renderer()
        : ctx{RenderContext::init()}
    {
    }
    RenderContext ctx;

    auto createSwapchain();
    auto recreateSwapchain(
        vk::raii::Queue                  &queue,
        uint32_t                         &currentFrame,
        std::vector<vk::Image>           &swapchainImages,
        std::vector<vk::raii::ImageView> &swapchainImageViews,
        std::vector<vk::raii::Semaphore> &imageAvailableSemaphores,
        std::vector<vk::raii::Semaphore> &renderFinishedSemaphores) -> void;
};

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format;
