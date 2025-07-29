#pragma once
#include <SDL3/SDL_video.h>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan_raii.hpp>

#include <VkBootstrap.h>

#include <fmt/base.h>

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

struct Frame {
    uint32_t                         currentFrameIndex = 0u;
    uint32_t                         maxFramesInFlight;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    Frame(
        vk::raii::Device &device,
        uint32_t          imagesSize);

    auto recreate(
        vk::raii::Device &device,
        uint32_t          imagesSize) -> void;
};

struct Swapchain {
    vk::raii::SwapchainKHR           swapchain;
    uint32_t                         nextImageIndex = 0u;
    std::vector<vk::Image>           images;
    std::vector<vk::raii::ImageView> imageViews;
    vk::Format                       imageFormat;
    vk::Format                       depthFormat;
    Frame                            frame;
    bool                             needRecreate = false;

    Swapchain(
        vk::raii::Device &device,
        vkb::Device      &vkbDevice);

    auto create(
        vk::raii::Device &device,
        vkb::Device      &vkbDevice) -> void;

    auto recreate(
        vk::raii::Device &device,
        vkb::Device      &vkbDevice,
        vk::raii::Queue  &queue) -> void;

    auto acquireNextImage() -> vk::Result;

    auto getNextImage() -> vk::Image;

    auto getNextImageView() -> vk::ImageView;

    auto getImageAvailableSemaphore() -> vk::Semaphore;

    auto getRenderFinishedSemaphore() -> vk::Semaphore;

    auto advanceFrame() -> void;
};

struct Renderer;

struct RenderContext {
    friend class Renderer;

    static constexpr auto SDL_WindowDeleter = [](SDL_Window *window) {
        SDL_DestroyWindow(window);
    };

    using WindowPtr = std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>;

    RenderContext(
        vk::raii::Context                context,
        vkb::Instance                    vkbInstance,
        vk::raii::Instance               instance,
        vk::raii::DebugUtilsMessengerEXT debugUtils,
        WindowPtr                        window,
        vk::raii::SurfaceKHR             surface,
        vkb::PhysicalDevice              vkbPhysicalDevice,
        vk::raii::PhysicalDevice         physicalDevice,
        vkb::Device                      vkbDevice,
        vk::raii::Device                 device,
        vk::raii::Queue                  graphicsQueue,
        vk::raii::Queue                  presentQueue,
        uint32_t                         graphicsQueueIndex,
        uint32_t                         presentQueueIndex,
        Swapchain                        swapchain,
        vk::Format                       depthFormat,
        vk::Extent2D                     windowExtent);

    ~RenderContext();

  private:
    static auto init() -> RenderContext;

  public:
    vk::raii::Context                context;
    vkb::Instance                    vkbInstance;
    vk::raii::Instance               instance;
    vk::raii::DebugUtilsMessengerEXT debugUtils;
    WindowPtr                        window;
    vk::raii::SurfaceKHR             surface;
    vkb::PhysicalDevice              vkbPhysicalDevice;
    vk::raii::PhysicalDevice         physicalDevice;
    vkb::Device                      vkbDevice;
    vk::raii::Device                 device;
    vk::raii::Queue                  graphicsQueue;
    vk::raii::Queue                  presentQueue;
    uint32_t                         graphicsQueueIndex;
    uint32_t                         presentQueueIndex;
    Swapchain                        swapchain;
    vk::Format                       depthFormat;

    vk::Extent2D windowExtent;
};

struct Renderer {
    friend struct RenderContext;
    Renderer();

    RenderContext ctx;

    auto present() -> void;
};

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format;
