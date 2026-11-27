#include "AABB.hpp"
#include "Camera.hpp"
#include "GltfLoader.hpp"
#include "ImGuiLayer.hpp"
#include "RenderContext.hpp"
#include "RenderableResources.hpp"
#include "Renderer.hpp"
#include "SceneView.hpp"
#include "ShaderInterfaceTypes.hpp"
#include "Utility.hpp"
#include "Window.hpp"

#include <SDL3/SDL_events.h>
#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <fmt/format.h>
#include <limits>

// TODO: add SDL event polling
void processEvents(bool& running)
{
}

auto chooseSurfaceFormat(std::vector<vk::SurfaceFormatKHR> const& availableFormats)
    -> vk::SurfaceFormatKHR
{
    constexpr auto preferredSurfaceFormats = std::array{
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eB8G8R8A8Unorm,
        vk::Format::eR8G8B8A8Snorm};

    constexpr auto preferredColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;

    for (auto preferredSurfaceFormat : preferredSurfaceFormats) {
        for (auto format : availableFormats) {
            if (format.format == preferredSurfaceFormat
                && format.colorSpace == preferredColorSpace) {
                return format;
            }
        }
    }

    return availableFormats.front();
}

auto updatePerFrameUniformBuffers(
    Allocator const&                     allocator,
    Buffer&                              frameUBO,
    Buffer&                              nodeInstancesSSBO,
    FrameUniforms const&                 frame,
    std::vector<NodeInstanceData> const& nodeInstances) -> void
{
    VK_CHECK(vmaCopyMemoryToAllocation(
        allocator.handle(),
        &frame,
        frameUBO.allocation,
        0,
        sizeof(FrameUniforms)));

    if (nodeInstances.empty()) {
        return;
    }

    auto const bytes = static_cast<vk::DeviceSize>(sizeof(NodeInstanceData))
                     * static_cast<vk::DeviceSize>(nodeInstances.size());

    VK_CHECK(vmaCopyMemoryToAllocation(
        allocator.handle(),
        nodeInstances.data(),
        nodeInstancesSSBO.allocation,
        0,
        bytes));
}

namespace
{

// Distance fit from nvpro_core2 CameraManipulator::fit (Apache-2.0):
// https://github.com/nvpro-samples/nvpro_core2/blob/main/nvutils/camera_manipulator.cpp
auto addDefaultCameraInstance(
    RenderAsset const& asset,
    SceneView&         sceneView,
    float              viewportAspect) -> void
{
    auto const  cameraIndex = static_cast<std::uint32_t>(asset.cameras.size() - 1u);
    auto const& camera = std::get<PerspectiveCamera>(asset.cameras[cameraIndex].model);

    auto const bounds      = computeSceneAABB(asset, sceneView);
    auto const boxHalfSize = 0.5f * (bounds.max - bounds.min);
    auto const boxCenter   = 0.5f * (bounds.min + bounds.max);

    auto const tanHalfFovY = std::tan(0.5f * camera.yfov);
    auto const tanHalfFovX = tanHalfFovY * viewportAspect;

    auto const radius        = glm::length(boxHalfSize);
    auto const idealDistance = std::max(radius / tanHalfFovX, radius / tanHalfFovY);

    auto const rotation =
        glm::angleAxis(glm::radians(-30.0f), glm::vec3{1.0f, 0.0f, 0.0f});
    auto const translation =
        boxCenter + rotation * glm::vec3{0.0f, 0.0f, idealDistance};

    sceneView.cameraInstances.push_back(
        CameraInstance{
            .translation = translation,
            .rotation    = rotation,
            .nodeIndex   = std::numeric_limits<std::uint32_t>::max(),
            .cameraIndex = cameraIndex,
        });
}

} // namespace
auto main(
    int   argc,
    char* argv[]) -> int
{
    auto gltfPath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "third_party/glTF-Sample-Assets/Models/Duck/glTF/Duck.gltf";
        } else {
            return argv[1];
        }
    }();

    auto gltfDirectory = gltfPath.parent_path();
    auto shaderPath    = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "build/debug/_autogen/mesh_basepass.slang.spv";
        } else {
            return std::string{argv[2]};
        }
    }();

    auto window             = createWindow(800, 600);
    auto requiredExtensions = std::vector{
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME,
        // VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME,
        VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME,
        VK_EXT_IMAGE_ROBUSTNESS_EXTENSION_NAME,
        VK_EXT_ROBUSTNESS_2_EXTENSION_NAME,
        VK_EXT_PIPELINE_ROBUSTNESS_EXTENSION_NAME,
        VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME,
        VK_EXT_HOST_IMAGE_COPY_EXTENSION_NAME,
        VK_EXT_MEMORY_BUDGET_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME};

    auto context = RenderContext{window, requiredExtensions};

    auto const colorFormat = context.colorFormat();
    auto const depthFormat = context.depthFormat();

    auto uploadCmd = Command{context.device, vk::CommandPoolCreateFlagBits::eTransient};

    auto asset         = getAsset(gltfPath);
    auto sceneDrawList = buildSceneView(asset);

    auto textureCount = static_cast<uint32_t>(asset.textures.size());

    auto rendererConfig = RendererConfig{
        .shaderInterfaceDescription = ShaderInterfaceDescription{{
            {kBindingFrameUniforms,
             vk::DescriptorType::eUniformBuffer,
             1,
             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},

            {kBindingNodeInstanceData,
             vk::DescriptorType::eStorageBuffer,
             1,
             vk::ShaderStageFlagBits::eVertex},

            {kBindingMaterialData,
             vk::DescriptorType::eStorageBuffer,
             1,
             vk::ShaderStageFlagBits::eFragment},

            {kBindingImagesSrgb,
             vk::DescriptorType::eSampledImage,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},

            {kBindingImagesLinear,
             vk::DescriptorType::eSampledImage,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},

            {kBindingSamplers,
             vk::DescriptorType::eSampler,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},
        }},
        .shaderPath                 = shaderPath,
        .colorFormat                = colorFormat,
        .depthFormat                = depthFormat,
        .maxFramesInFlight          = 2,
    };
    auto renderer = Renderer{context, rendererConfig};

    auto ui = ImGuiLayer{window.get(), context, rendererConfig.maxFramesInFlight};

    auto const initialExtent         = context.extent();
    auto const initialViewportAspect = static_cast<float>(initialExtent.width)
                                     / static_cast<float>(initialExtent.height);

    addDefaultCameraInstance(asset, sceneDrawList, initialViewportAspect);
    auto renderableResources = RenderableResources{};
    renderableResources
        .create(asset, sceneDrawList, context.device, uploadCmd, context.allocator);

    auto nodeInstancesData = std::vector<NodeInstanceData>{};
    nodeInstancesData.reserve(sceneDrawList.nodeInstances.size());

    for (auto const& nodeInstance : sceneDrawList.nodeInstances) {
        nodeInstancesData.push_back(
            NodeInstanceData{
                .modelMatrix = nodeInstance.modelMatrix,
            });
    }

    auto const nodeInstanceCount = static_cast<uint32_t>(nodeInstancesData.size());
    renderer.initializePerFrameUniformBuffers(context.allocator, nodeInstanceCount);

    auto     running        = true;
    auto     previousTime   = std::chrono::high_resolution_clock::now();
    auto     cumulativeTime = previousTime - previousTime;
    uint64_t frameCount     = 0;

    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            else if (e.type == SDL_EVENT_KEY_DOWN && e.key.key == SDLK_RIGHTBRACKET) {
                renderer.cycleDebugView();
            }
        }

        if (!renderer.beginFrame()) {
            continue;
        }

        ui.beginFrame();
        ImGui::ShowDemoWindow();

        auto currentTime = std::chrono::high_resolution_clock::now();
        auto dt          = currentTime - previousTime;
        previousTime     = currentTime;

        cumulativeTime += dt;
        ++frameCount;

        using namespace std::chrono_literals;

        if (cumulativeTime > 3s) {
            auto fps = frameCount * 1000000000UL / cumulativeTime.count();
            fmt::println(stderr, "{} FPS ({:.2} ms)", fps, 1000.0 / fps);

            cumulativeTime -= 3s;
            frameCount = 0;
        }

        auto const extent = context.extent();
        auto const viewportAspect =
            static_cast<float>(extent.width) / static_cast<float>(extent.height);

        auto const& activeCameraInstance = sceneDrawList.activeCameraInstance();

        auto const& activeCamera = asset.cameras[activeCameraInstance.cameraIndex];

        auto frameUniforms = FrameUniforms{
            .viewProjectionMatrix = activeCamera.getProjectionMatrix(viewportAspect)
                                  * activeCamera.getViewMatrix(
                                      activeCameraInstance.translation,
                                      activeCameraInstance.rotation),
            .directionalLight     = DirectionalLight{},
            .pointLight           = PointLight{},
        };

        updatePerFrameUniformBuffers(
            context.allocator,
            renderer.currentFrameUBO(),
            renderer.currentNodeInstancesSSBO(),
            frameUniforms,
            nodeInstancesData);

        renderableResources.updateDescriptorSet(
            context.device,
            renderer.currentDescriptorSet(),
            renderer.currentFrameUBO(),
            renderer.currentNodeInstancesSSBO());

        renderer.render(renderableResources);
        renderer.beginUiPass();
        ui.record(renderer.cmd());
        renderer.endUiPass();
        renderer.endFrame();
    }

    context.device.graphicsQueue.waitIdle();
}
