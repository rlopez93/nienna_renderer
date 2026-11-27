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

    fmt::println(stderr, "[imgui vulkan] VkResult = {}", static_cast<int>(result));

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
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL3_InitForVulkan(window)) {
        ImGui::DestroyContext();
        throw std::runtime_error{"ImGui_ImplSDL3_InitForVulkan failed"};
    }

    auto const colorFormat = static_cast<VkFormat>(context.colorFormat());

    auto initInfo                = ImGui_ImplVulkan_InitInfo{};
    initInfo.ApiVersion          = VK_API_VERSION_1_4;
    initInfo.Instance            = *context.instance.handle;
    initInfo.PhysicalDevice      = *context.physicalDevice.handle;
    initInfo.Device              = *context.device.handle;
    initInfo.QueueFamily         = context.device.queueFamilyIndices.graphicsIndex;
    initInfo.Queue               = *context.device.graphicsQueue;
    initInfo.DescriptorPoolSize  = IMGUI_IMPL_VULKAN_MINIMUM_IMAGE_SAMPLER_POOL_SIZE;
    initInfo.MinImageCount       = 2u;
    initInfo.ImageCount          = std::max(framesInFlight, initInfo.MinImageCount);
    initInfo.MSAASamples         = VK_SAMPLE_COUNT_1_BIT;
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount    = 1u,
        .pColorAttachmentFormats = &colorFormat,
    };
    initInfo.MinAllocationSize = 1024u * 1024u;
    initInfo.CheckVkResultFn   = checkVkResult;

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
