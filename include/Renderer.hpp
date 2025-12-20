// Renderer.hpp
#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "FrameContext.hpp"
#include "RenderContext.hpp"
#include "RendererConfig.hpp"
#include "ShaderInterface.hpp"
#include "Swapchain.hpp"

struct SceneResources;

struct Renderer {

    Renderer(
        RenderContext        &context,
        const RendererConfig &config);

    auto allocateFrameDescriptorSets(FrameContext &frames);

    [[nodiscard]]
    auto beginFrame() -> bool;

    auto render(const SceneResources &sceneResources) -> void;

    auto endFrame() -> void;

    [[nodiscard]]
    auto getWindowExtent() const -> vk::Extent2D;

  private:
    RenderContext &context;

    // Renderer-owned execution state
    Swapchain                swapchain;
    FrameContext             frames;
    ShaderInterface          shaderInterface;
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline       graphicsPipeline;

  private:
    auto submit() -> void;
    auto present() -> void;
};
