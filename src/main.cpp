#include "GltfLoader.hpp"
#include "RenderContext.hpp"
#include "Renderer.hpp"
#include "SceneRenderData.hpp"
#include "ShaderInterfaceTypes.hpp"
#include "Utility.hpp"
#include "Window.hpp"

#include <SDL3/SDL_events.h>

// TODO: add SDL event polling
void processEvents(bool &running)
{
}

auto chooseSurfaceFormat(const std::vector<vk::SurfaceFormatKHR> &availableFormats)
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

auto updatePerFrameBuffers(
    const Allocator               &allocator,
    Buffer                        &frameUbo,
    Buffer                        &objectsSsbo,
    const FrameUniforms           &frame,
    const std::vector<ObjectData> &objects) -> void
{
    VK_CHECK(vmaCopyMemoryToAllocation(
        allocator.allocator,
        &frame,
        frameUbo.allocation,
        0,
        sizeof(FrameUniforms)));

    if (objects.empty()) {
        return;
    }

    const auto bytes = static_cast<vk::DeviceSize>(sizeof(ObjectData))
                     * static_cast<vk::DeviceSize>(objects.size());

    VK_CHECK(vmaCopyMemoryToAllocation(
        allocator.allocator,
        objects.data(),
        objectsSsbo.allocation,
        0,
        bytes));
}

auto main(
    int   argc,
    char *argv[]) -> int
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
            return "build/_autogen/mesh-refactor.slang.spv";
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

    auto asset = getAsset(gltfPath);

    const auto formats =
        context.physicalDevice.handle.getSurfaceFormatsKHR(context.surface.handle);

    const auto surfaceFormat = chooseSurfaceFormat(formats);
    const auto depthFormat   = findDepthFormat(context.physicalDevice.handle);

    context.depth.format = depthFormat;

    Command uploadCmd{context.device, vk::CommandPoolCreateFlagBits::eTransient};

    context.depth.recreate(
        context.device,
        context.allocator,
        uploadCmd,
        context.swapchain.extent());

    auto textureCount = static_cast<uint32_t>(asset.textures.size());

    RendererConfig rendererConfig{
        .shaderInterfaceDescription = ShaderInterfaceDescription{{
            {0,
             vk::DescriptorType::eUniformBuffer,
             1,
             vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment},

            {1,
             vk::DescriptorType::eStorageBuffer,
             1,
             vk::ShaderStageFlagBits::eVertex},

            {2,
             vk::DescriptorType::eStorageBuffer,
             1,
             vk::ShaderStageFlagBits::eFragment},

            {3,
             vk::DescriptorType::eSampledImage,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},

            {4,
             vk::DescriptorType::eSampledImage,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},

            {5,
             vk::DescriptorType::eSampler,
             textureCount,
             vk::ShaderStageFlagBits::eFragment},
        }},
        .shaderPath                 = shaderPath,
        .colorFormat                = surfaceFormat.format,
        .depthFormat                = depthFormat,
        .maxFramesInFlight          = 2,
    };
    auto renderer = Renderer{context, rendererConfig};

    auto sceneRenderData = SceneRenderData{};
    sceneRenderData.create(scene, context.device, uploadCmd, context.allocator);
    // TODO: parse sampler from .gltf
    auto sampler = vk::raii::Sampler{context.device.handle, vk::SamplerCreateInfo{}};

    renderer.initializePerFrameUniforms(context.allocator, scene.meshes.size());

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

            scene.processInput(e);
        }

        if (!renderer.beginFrame()) {
            continue;
        }

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

        scene.update(dt);

        updatePerFrameUniformBuffers(
            context.allocator,
            renderer.currentTransformUBO(),
            renderer.currentLightUBO(),
            scene);

        sceneRenderData.updateDescriptorSet(
            context.device,
            renderer.currentDescriptorSet(),
            renderer.currentTransformUBO(),
            renderer.currentLightUBO(),
            sampler);

        renderer.render(sceneRenderData);

        renderer.endFrame();
    }

    context.device.graphicsQueue.waitIdle();
}
