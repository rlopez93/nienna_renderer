#include "RenderContext.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "SceneResources.hpp"
#include "Window.hpp"
#include "gltfLoader.hpp"

// TODO: add SDL event polling
void processEvents(bool &running)
{
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
        VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
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

    // TODO: populate shader interface bindings
    auto shaderInterfaceDescription =
        ShaderInterfaceDescription{std::vector<ShaderInterfaceDescription::Binding>{}};

    auto rendererConfig = RendererConfig{
        shaderInterfaceDescription,
        shaderPath,
        vk::Format{}, // TODO: color format
        vk::Format{}, // TODO: depth format
        {}            // TODO: maxFramesInFlight
    };

    auto renderer = Renderer{context, rendererConfig};
    auto asset    = getGltfAsset(gltfPath);
    auto scene    = getSceneData(asset, gltfDirectory);

    // TODO: parse sampler from .gltf
    auto sampler = vk::raii::Sampler{context.device.handle, vk::SamplerCreateInfo{}};
    auto sceneResources = SceneResources{};
    bool running        = true;
    auto startTime      = std::chrono::high_resolution_clock::now();
    auto previousTime   = startTime;
    auto dt             = startTime - previousTime;

    while (running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        dt               = currentTime - previousTime;
        processEvents(running);
        if (!renderer.beginFrame()) {
            continue;
        }

        previousTime = currentTime;

        scene.update(dt);
        sceneResources.updateDescriptorSet(
            context.device,

            {},
            scene.meshes.size(),
            sampler);

        renderer.render(sceneResources);
        renderer.endFrame();
    }
}
