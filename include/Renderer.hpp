#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "FrameContext.hpp"
#include "RenderContext.hpp"
#include "RendererConfig.hpp"
#include "ShaderInterface.hpp"

struct SceneResources;

struct Renderer {
    Renderer(
        RenderContext        &context,
        const RendererConfig &config);

    // Returns false if the frame should be skipped (e.g. swapchain
    // out-of-date/minimized)
    [[nodiscard]]
    auto beginFrame() -> bool;

    // Records rendering commands into the current frame command buffer
    auto render(const SceneResources &sceneResources) -> void;

    // Submits + presents + advances frame
    auto endFrame() -> void;

  private:
    RenderContext &context;

    // Renderer-owned execution state
    FrameContext frames;

    // Shader contract + pipeline state
    ShaderInterface          shaderInterface;
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline       graphicsPipeline;

    // Internal mechanics
    auto allocateFrameDescriptorSets() -> void;
    auto submit() -> void;
    auto present() -> void;
};
