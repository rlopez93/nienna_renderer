#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL_init.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/base.h>

#include <vector>

#include "Allocator.hpp"
#include "Descriptor.hpp"
#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "Timeline.hpp"
#include "gltfLoader.hpp"

struct Renderer {

    Renderer();
    ~Renderer();

    auto drawFrame(
        Scene                             &scene,
        const std::chrono::duration<float> deltaTime,
        vk::raii::Pipeline                &pipeline,
        Descriptors                       &descriptors,
        bool                               framebufferResized) -> void;

    auto beginFrame() -> void;

    auto beginRender(
        vk::AttachmentLoadOp  loadOp,
        vk::AttachmentStoreOp storeOp,
        vk::AttachmentLoadOp  depthLoadOp,
        vk::AttachmentStoreOp depthStoreOp) -> void;

    auto render(
        Scene              &scene,
        vk::raii::Pipeline &pipeline,
        Descriptors        &descriptors) -> void;

    auto endRender() -> void;

    auto submit() -> void;

    auto present() -> void;

    auto endFrame() -> void;

    [[nodiscard]]
    auto getWindowExtent() const -> vk::Extent2D;

    Window                              window;
    Instance                            instance;
    Surface                             surface;
    std::vector<const char *>           requiredExtensions;
    PhysicalDevice                      physicalDevice;
    Device                              device;
    Swapchain                           swapchain;
    Allocator                           allocator;
    vk::Format                          depthFormat;
    Image                               depthImage;
    vk::raii::ImageView                 depthImageView;
    Timeline                            timeline;
    std::vector<vk::DescriptorPoolSize> poolSizes;
    vk::raii::DescriptorPool            imguiDescriptorPool;
};
