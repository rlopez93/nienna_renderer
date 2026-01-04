#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "DebugView.hpp"
#include "FrameContext.hpp"
#include "LayoutTracker.hpp"
#include "RenderContext.hpp"
#include "RendererConfig.hpp"
#include "ShaderInterface.hpp"

struct SceneRenderData;

struct Renderer {
    Renderer(
        RenderContext        &context,
        const RendererConfig &config);

    // Returns false if the frame should be skipped (e.g. swapchain
    // out-of-date/minimized)
    [[nodiscard]]
    auto beginFrame() -> bool;

    // Records rendering commands into the current frame command buffer
    auto render(const SceneRenderData &sceneRenderData) -> void;

    // Submits + presents + advances frame
    auto endFrame() -> void;

    // Frame-relative resource access (read/write by application)
    vk::DescriptorSet currentDescriptorSet() const;
    Buffer           &currentFrameUBO();
    Buffer           &currentNodeInstancesSSBO();

    auto initializePerFrameUniformBuffers(
        Allocator &allocator,
        uint32_t   nodeInstancesCount) -> void;

    void cycleDebugView();

  private:
    RenderContext &context;
    LayoutTracker  layoutTracker;

    ShaderInterface          shaderInterface;
    vk::raii::DescriptorPool descriptorPool;
    vk::raii::PipelineLayout pipelineLayout;
    vk::raii::Pipeline       graphicsPipeline;

    // Renderer-owned execution state
    FrameContext frames;

    DebugView debugView = DebugView::Shaded;

    auto        submit() -> void;
    auto        present() -> void;
    auto        allocateFrameDescriptorSets() -> void;
    static auto createDescriptorPool(
        Device                           &device,
        const ShaderInterfaceDescription &interfaceDesc,
        uint32_t maxFramesInFlight) -> vk::raii::DescriptorPool;
};
