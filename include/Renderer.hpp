#pragma once
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>

#include <fmt/base.h>

#include <memory>
#include <ranges>
#include <utility>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan_raii.hpp>

#include <VkBootstrap.h>

struct Frame {
    uint32_t                         currentFrameIndex = 0u;
    uint32_t                         maxFramesInFlight;
    std::vector<vk::raii::Semaphore> imageAvailableSemaphores;
    std::vector<vk::raii::Semaphore> renderFinishedSemaphores;

    Frame(
        vk::raii::Device &device,
        uint32_t          imagesSize)
    {
        recreate(device, imagesSize);
    }

    auto recreate(
        vk::raii::Device &device,
        uint32_t          imagesSize) -> void
    {
        currentFrameIndex = 0u;
        imageAvailableSemaphores.clear();
        renderFinishedSemaphores.clear();

        maxFramesInFlight = imagesSize;
        for (auto i : std::views::iota(0u, maxFramesInFlight)) {
            imageAvailableSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
            renderFinishedSemaphores.emplace_back(device, vk::SemaphoreCreateInfo{});
        }
    }
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
        vkb::Device      &vkbDevice)
        : swapchain(nullptr),
          frame(
              device,
              0u)
    {
        create(device, vkbDevice);
    }

    auto create(
        vk::raii::Device &device,
        vkb::Device      &vkbDevice) -> void
    {
        auto swapchainBuilder = vkb::SwapchainBuilder{vkbDevice};
        auto swapchainResult =
            swapchainBuilder
                //.set_old_swapchain(*swapchain)
                .set_image_usage_flags(
                    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                    | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
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

        if (!swapchainResult) {
            fmt::print(
                stderr,
                "vk-bootstrap failed to create swapchain: {}",
                swapchainResult.error().message());
            throw std::exception{};
        }

        swapchain = vk::raii::SwapchainKHR{device, swapchainResult->swapchain};
        images    = swapchain.getImages();
        imageViews.clear();
        imageFormat = vk::Format(swapchainResult->image_format);

        for (auto image : images) {
            imageViews.emplace_back(
                device,
                vk::ImageViewCreateInfo{
                    {},
                    image,
                    vk::ImageViewType::e2D,
                    imageFormat,
                    {},
                    {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
        }

        frame.recreate(device, images.size());
    }

    auto recreate(
        vk::raii::Device &device,
        vkb::Device      &vkbDevice,
        vk::raii::Queue  &queue) -> void

    {
        queue.waitIdle();

        frame.currentFrameIndex = 0;

        swapchain.clear();

        create(device, vkbDevice);

        needRecreate = false;
    }

    auto acquireNextImage()
    {
        vk::Result result;
        std::tie(result, nextImageIndex) = swapchain.acquireNextImage(
            std::numeric_limits<uint64_t>::max(),
            getImageAvailableSemaphore());

        return result;
    }

    auto getNextImage()
    {
        return images[nextImageIndex];
    }

    auto getNextImageView() -> vk::ImageView
    {
        return imageViews[nextImageIndex];
    }

    auto getImageAvailableSemaphore() -> vk::Semaphore
    {
        return frame.imageAvailableSemaphores[frame.currentFrameIndex];
    }

    auto getRenderFinishedSemaphore() -> vk::Semaphore
    {
        return frame.renderFinishedSemaphores[nextImageIndex];
    }

    auto advanceFrame()
    {

        frame.currentFrameIndex =
            (frame.currentFrameIndex + 1) % frame.maxFramesInFlight;
    }
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
        vk::Extent2D                     windowExtent)
        : context{std::move(context)},
          vkbInstance{vkbInstance},
          instance{std::move(instance)},
          debugUtils{std::move(debugUtils)},
          window{std::move(window)},
          surface{std::move(surface)},
          vkbPhysicalDevice{std::move(vkbPhysicalDevice)},
          physicalDevice{std::move(physicalDevice)},
          vkbDevice{std::move(vkbDevice)},
          device{std::move(device)},
          graphicsQueue{std::move(graphicsQueue)},
          presentQueue{std::move(presentQueue)},
          graphicsQueueIndex{graphicsQueueIndex},
          presentQueueIndex{presentQueueIndex},
          swapchain{std::move(swapchain)},
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
    vk::Extent2D                     windowExtent;
};

struct Renderer {
    friend struct RenderContext;
    Renderer()
        : ctx{RenderContext::init()}
    {
    }

    RenderContext ctx;

    auto present()
    {
        auto renderFinishedSemaphore = ctx.swapchain.getRenderFinishedSemaphore();
        auto presentResult           = ctx.presentQueue.presentKHR(
            vk::PresentInfoKHR{
                renderFinishedSemaphore,
                *ctx.swapchain.swapchain,
                ctx.swapchain.nextImageIndex});

        if (presentResult == vk::Result::eErrorOutOfDateKHR
            || presentResult == vk::Result::eSuboptimalKHR) {
            ctx.swapchain.needRecreate = true;
        }

        else if (!(presentResult == vk::Result::eSuccess
                   || presentResult == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        // advance to the next frame in the swapchain
        ctx.swapchain.advanceFrame();
    }
};

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format;
