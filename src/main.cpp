
#include "vulkan_raii.hpp"

#include "App.hpp"
#include "Renderer.hpp"

#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_vulkan.h"

#include <SDL3/SDL_events.h>
#include <fmt/base.h>

#include <numeric>
#include <ranges>

auto main(
    int    argc,
    char **argv) -> int
{
    // VULKAN_HPP_DEFAULT_DISPATCHER.init();
    auto filePath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "third_party/glTF-Sample-Assets/Models/Duck/glTF/Duck.gltf";
        } else {
            return std::string{argv[1]};
        }
    }();

    auto gltfDirectory = filePath.parent_path();
    auto gltfFilename  = filePath.filename();

    auto shaderPath = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "build/_autogen/mesh-refactor.slang.spv";
        } else {
            return std::string{argv[2]};
        }
    }();

    // instantiate our renderer
    // this constructor does A LOT
    Renderer r;

    // create transient command pool for single-time commands
    auto transient =
        Command{r.device, r.graphicsQueue, vk::CommandPoolCreateFlagBits::eTransient};

    // Transition image layout
    transient.beginSingleTime();

    for (auto image : r.swapchain.images) {
        cmdTransitionImageLayout(
            transient.buffer,
            image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageAspectFlagBits::eColor);
    }

    transient.endSingleTime(r.device, r.graphicsQueue);

    auto asset = getGltfAsset(gltfDirectory / gltfFilename);
    auto scene = getSceneData(asset, gltfDirectory);

    // TODO: get sampler from glTF asset
    auto sampler = vk::raii::Sampler{r.device.handle, vk::SamplerCreateInfo{}};

    scene.createBuffersOnDevice(
        r.device,
        transient,
        r.allocator,
        r.graphicsQueue,
        r.swapchain.frame.maxFramesInFlight);

    uint32_t textureCount = scene.textureBuffers.image.size();
    uint32_t meshCount    = scene.meshes.size();
    fmt::println(stderr, "textures.size(): {}", textureCount);
    fmt::println(stderr, "meshes.size(): {}", meshCount);

    auto descriptors = Descriptors{
        r.device.handle,
        meshCount,
        textureCount,
        r.swapchain.frame.maxFramesInFlight};

    updateDescriptorSets(
        r.device.handle,
        descriptors,
        scene.buffers.uniform,
        meshCount,
        scene.textureBuffers.imageView,
        sampler);

    auto graphicsPipeline = createPipeline(
        r.device.handle,
        shaderPath,
        r.swapchain.imageFormat,
        r.depthFormat,
        descriptors.pipelineLayout);

    uint32_t totalFrames = 0;

    using namespace std::literals;

    auto           start        = std::chrono::high_resolution_clock::now();
    auto           previousTime = start;
    auto           currentTime  = start;
    auto           runningTime  = 0.0s;
    bool           running      = true;
    constexpr auto period       = 1.0s;

    SDL_Event e;
    while (running) {
        currentTime    = std::chrono::high_resolution_clock::now();
        auto deltaTime = currentTime - previousTime;
        runningTime += deltaTime;

        if (runningTime > period) {
            auto fps =
                totalFrames
                / std::chrono::duration_cast<std::chrono::seconds>(currentTime - start)
                      .count();
            fmt::println(stderr, "{} fps", fps);
            runningTime -= period;
        }

        previousTime = currentTime;

        while (SDL_PollEvent(&e)) {

            ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            scene.processInput(e);
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow();

        ImGui::Render();
        auto imGuiDrawData = ImGui::GetDrawData();

        scene.update(deltaTime);

        // FIXME:
        // Does copying uniform data to device go in scene.update()?
        // Synchronization?

        // update uniform buffers
        for (const auto &[meshIndex, mesh] : std::views::enumerate(scene.meshes)) {
            auto transform = Transform{
                .modelMatrix          = mesh.modelMatrix,
                .viewProjectionMatrix = scene.getCamera().getProjectionMatrix()
                                      * scene.getCamera().getViewMatrix()};
            // transform.viewProjectionMatrix[1][1] *= -1;

            VK_CHECK(vmaCopyMemoryToAllocation(
                r.allocator.allocator,
                &transform,
                scene.buffers.uniform[r.swapchain.frame.index].allocation,
                sizeof(Transform) * meshIndex,
                sizeof(Transform)));
        }

        r.beginFrame();

        // render scene
        r.render(scene, graphicsPipeline, descriptors);

        // render imGui
        r.beginRender(
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore,
            vk::AttachmentLoadOp::eLoad,
            vk::AttachmentStoreOp::eStore);

        // ImGui_ImplVulkan_RenderDrawData(imGuiDrawData, *r.timeline.buffer());

        r.endRender();

        r.submit();
        r.present();

        r.endFrame();
        ImGui::EndFrame();

        ++totalFrames;
    }

    r.device.handle.waitIdle();

    return 0;
}
