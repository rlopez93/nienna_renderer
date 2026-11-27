#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "DebugView.hpp"
#include "FrameContext.hpp"
#include "ImageLayoutState.hpp"
#include "RenderContext.hpp"
#include "RendererConfig.hpp"
#include "ShaderInterface.hpp"

struct RenderableResources;

struct Renderer {
    Renderer(
        RenderContext&        context,
        RendererConfig const& config);

    // Returns false if the frame should be skipped (e.g. swapchain
    // out-of-date/minimized)
    [[nodiscard]]
    auto beginFrame() -> bool;

    // Records rendering commands into the current frame command buffer
    auto render(RenderableResources const& renderableResources) -> void;

    auto beginUiPass() -> void;
    auto endUiPass() -> void;

    auto commandBuffer() -> vk::raii::CommandBuffer&;

    // Submits + presents + advances frame
    auto endFrame() -> void;

    // Frame-relative resource access (read/write by application)
    auto currentDescriptorSet() const -> vk::DescriptorSet;
    auto currentFrameUBO() -> Buffer&;
    auto currentNodeInstancesSSBO() -> Buffer&;

    auto initializePerFrameUniformBuffers(
        Allocator& allocator,
        uint32_t   nodeInstancesCount) -> void;

    void cycleDebugView();

  private:
    RenderContext&   context;
    ImageLayoutState imageLayoutState;

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
        Device&                           device,
        ShaderInterfaceDescription const& interfaceDesc,
        uint32_t maxFramesInFlight) -> vk::raii::DescriptorPool;
};
