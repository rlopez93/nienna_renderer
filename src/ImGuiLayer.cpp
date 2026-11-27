#include "ImGuiLayer.hpp"

#include "RenderContext.hpp"

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_vulkan.h>

#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <stdexcept>

namespace
{

auto checkVkResult(VkResult result) -> void
{
    if (result == VK_SUCCESS) {
        return;
    }

    fmt::println(
        stderr,
        "[imgui vulkan] {}",
        vk::to_string(static_cast<vk::Result>(result)));

    if (result < 0) {
        std::abort();
    }
}

} // namespace

ImGuiLayer::ImGuiLayer(
    SDL_Window*    window,
    RenderContext& context,
    std::uint32_t  framesInFlight)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        ImGui::DestroyContext();
        throw std::runtime_error{"ImGui_ImplSDL3_InitForVulkan failed"};
    }

    auto const colorFormat = context.colorFormat();

    auto initInfo = ImGui_ImplVulkan_InitInfo{
        .ApiVersion     = vk::ApiVersion14,
        .Instance       = *context.instance.handle,
        .PhysicalDevice = *context.physicalDevice.handle,
        .Device         = *context.device.handle,
        .QueueFamily    = context.device.queueFamilyIndices.graphicsIndex,
        .Queue          = *context.device.graphicsQueue,
        .MinImageCount  = 2u,
        .ImageCount     = std::max(framesInFlight, 2u),
        .MSAASamples = static_cast<VkSampleCountFlagBits>(vk::SampleCountFlagBits::e1),
        .DescriptorPoolSize  = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE,
        .UseDynamicRendering = true,
        .PipelineRenderingCreateInfo =
            vk::PipelineRenderingCreateInfo{}.setColorAttachmentFormats(colorFormat),
        .CheckVkResultFn   = checkVkResult,
        .MinAllocationSize = 1024u * 1024u,
    };

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
        throw std::runtime_error{"ImGui_ImplVulkan_Init failed"};
    }
}

auto ImGuiLayer::beginFrame() -> void
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

auto ImGuiLayer::record(vk::raii::CommandBuffer& cmd) -> void
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
}

ImGuiLayer::~ImGuiLayer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
}
