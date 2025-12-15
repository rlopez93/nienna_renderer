
#include <vulkan/vulkan_raii.hpp>

#include "App.hpp"
#include "Command.hpp"
#include "Renderer.hpp"
#include "gltfLoader.hpp"

#include <SDL3/SDL_events.h>
#include <fmt/base.h>

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
    auto transient = Command{
        r.device,
        vk::CommandPoolCreateFlagBits::eTransient
            | vk::CommandPoolCreateFlagBits::eResetCommandBuffer};

    auto asset = getGltfAsset(gltfDirectory / gltfFilename);
    auto scene = getSceneData(asset, gltfDirectory);

    // TODO: get sampler from glTF asset
    auto sampler = vk::raii::Sampler{r.device.handle, vk::SamplerCreateInfo{}};

    scene.createBuffersOnDevice(
        r.device,
        transient,
        r.allocator,
        r.swapchain.frame.maxFramesInFlight);

    uint32_t textureCount = scene.textureBuffers.image.size();
    uint32_t meshCount    = scene.meshes.size();
    // fmt::println(stderr, "textures.size(): {}", textureCount);
    // fmt::println(stderr, "meshes.size(): {}", meshCount);

    auto descriptors = Descriptors{
        r.device.handle,
        meshCount,
        textureCount,
        r.swapchain.frame.maxFramesInFlight};

    updateDescriptorSets(
        r.device.handle,
        descriptors,
        scene.buffers.uniform,
        scene.buffers.light,
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
    bool      framebufferResized = false;
    while (running) {
        // fmt::println(stdout, "Frame {}", totalFrames);
        currentTime    = std::chrono::high_resolution_clock::now();
        auto deltaTime = currentTime - previousTime;
        runningTime += deltaTime;

        // if (runningTime > period) {
        //     auto fps =
        //         totalFrames
        //         / std::chrono::duration_cast<std::chrono::seconds>(currentTime -
        //         start)
        //               .count();
        //     fmt::println(stderr, "{} fps", fps);
        //     runningTime -= period;
        // }

        previousTime = currentTime;

        framebufferResized = false;

        // TODO: move into Input.hpp or something
        while (SDL_PollEvent(&e)) {

            // ImGui_ImplSDL3_ProcessEvent(&e);

            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }

            else if (e.type == SDL_EVENT_WINDOW_RESIZED) {
                framebufferResized = true;
                // fmt::println(stderr, "Window resized!");
            }

            else if (e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                framebufferResized = true;
                // fmt::println(stderr, "Window pixel size changed!");

            } else if (e.type == SDL_EVENT_WINDOW_MINIMIZED) {
                // rendering will pause automatically
                // fmt::println(stderr, "Window minimized!");

            } else if (e.type == SDL_EVENT_WINDOW_RESTORED) {
                framebufferResized = true;
                // fmt::println(stderr, "Window restored!");
            }

            scene.processInput(e);
        }

        r.drawFrame(
            scene,
            deltaTime,
            graphicsPipeline,
            descriptors,
            framebufferResized);
    }

    r.device.handle.waitIdle();

    return 0;
}
