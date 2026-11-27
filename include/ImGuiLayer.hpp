#pragma once

#include <SDL3/SDL_video.h>
#include <vulkan/vulkan_raii.hpp>

#include <cstdint>

struct RenderContext;

struct ImGuiLayer {
    ImGuiLayer(
        SDL_Window*    window,
        RenderContext& context,
        std::uint32_t  framesInFlight);
    ~ImGuiLayer();

    ImGuiLayer(ImGuiLayer const&)                    = delete;
    auto operator=(ImGuiLayer const&) -> ImGuiLayer& = delete;
    ImGuiLayer(ImGuiLayer&&)                         = delete;
    auto operator=(ImGuiLayer&&) -> ImGuiLayer&      = delete;

    auto beginFrame() -> void;

    auto record(vk::raii::CommandBuffer& cmd) -> void;
};
