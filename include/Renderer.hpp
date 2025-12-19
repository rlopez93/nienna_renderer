#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"
#include "FrameContext.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Pipeline.hpp"
#include "PipelineLayout.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"

#include <vector>

struct SceneResources;

struct Renderer {

    Renderer();
    ~Renderer();

    [[nodiscard]]
    auto beginFrame() -> bool;
    auto render(const SceneResources &sceneResources) -> void;
    auto endFrame() -> void;

    [[nodiscard]]
    auto getWindowExtent() const -> vk::Extent2D;

    Window                              window;
    Instance                            instance;
    Surface                             surface;
    std::vector<const char *>           requiredExtensions;
    PhysicalDevice                      physicalDevice;
    Device                              device;
    FrameContext                        frames;
    Swapchain                           swapchain;
    vk::raii::PipelineLayout            pipelineLayout;
    vk::raii::Pipeline                  graphicsPipeline;
    std::vector<vk::DescriptorPoolSize> poolSizes;
    vk::raii::DescriptorPool            imguiDescriptorPool;

  private:
    auto submit() -> void;
    auto present() -> void;
};
