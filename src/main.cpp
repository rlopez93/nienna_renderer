#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE;

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>

#include <fastgltf/core.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <SPIRV-Reflect/spirv_reflect.h>

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <fmt/base.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>

struct Vertex;

auto readShaders(const std::string &filename) -> std::vector<char>;
auto getGltfAsset(const std::filesystem::path &gltfPath)
    -> fastgltf::Expected<fastgltf::Asset>;
auto getGltfAssetData(fastgltf::Asset &asset)
    -> std::pair<std::vector<uint16_t>, std::vector<Vertex>>;

auto beginSingleTimeCommands(
    vk::raii::Device &device, vk::raii::CommandPool &commandPool)
    -> vk::raii::CommandBuffer;
auto endSingleTimeCommands(
    vk::raii::Device &device, vk::raii::CommandPool &commandPool,
    vk::CommandBuffer &commandBuffer, vk::Queue &queue);

auto endSingleTimeCommands(
    vk::raii::Device &device, vk::raii::CommandPool &commandPool,
    vk::CommandBuffer &commandBuffer, vk::raii::Queue &queue)
{
    // submit command buffer
    commandBuffer.end();

    // create fence for synchronization
    auto fence                   = vk::raii::Fence{device, {}};
    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{commandBuffer};

    queue.submit2(vk::SubmitInfo2{{}, {}, commandBufferSubmitInfo}, fence);
    auto result =
        device.waitForFences({fence}, true, std::numeric_limits<uint64_t>::max());

    assert(result == vk::Result::eSuccess);
}

auto beginSingleTimeCommands(
    vk::raii::Device &device, vk::raii::CommandPool &commandPool)
    -> vk::raii::CommandBuffer
{
    auto commandBuffers = vk::raii::CommandBuffers{
        device,
        vk::CommandBufferAllocateInfo{
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            1}};
    commandBuffers.front().begin(
        vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return std::move(commandBuffers.front());
};

struct Vertex {
    fastgltf::math::fvec3 pos{};
    fastgltf::math::fvec3 normal{};
    fastgltf::math::fvec2 texCoord{};
};

auto getGltfAsset(const std::filesystem::path &gltfPath)
    -> fastgltf::Expected<fastgltf::Asset>
{
    constexpr auto supportedExtensions = fastgltf::Extensions::KHR_mesh_quantization
                                       | fastgltf::Extensions::KHR_texture_transform
                                       | fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser(supportedExtensions);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages
        | fastgltf::Options::GenerateMeshIndices;

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(gltfPath);
    if (!bool(gltfFile)) {
        fmt::print(
            stderr,
            "Failed to open glTF file: {}\n",
            fastgltf::getErrorMessage(gltfFile.error()));
        return fastgltf::Error{};
    }

    auto asset = parser.loadGltf(gltfFile.get(), gltfPath.parent_path(), gltfOptions);

    if (fastgltf::getErrorName(asset.error()) != "None") {
        fmt::print(
            stderr,
            "Failed to open glTF file: {}\n",
            fastgltf::getErrorMessage(gltfFile.error()));
    }

    return asset;
}

auto getGltfAssetData(fastgltf::Asset &asset)
    -> std::pair<std::vector<uint16_t>, std::vector<Vertex>>
{
    auto indices  = std::vector<uint16_t>{};
    auto vertices = std::vector<Vertex>{};
    for (auto &mesh : asset.meshes) {
        fmt::print(stderr, "Mesh is: <{}>\n", mesh.name);
        for (auto primitiveIt = mesh.primitives.begin();
             primitiveIt != mesh.primitives.end();
             ++primitiveIt) {

            if (primitiveIt->indicesAccessor.has_value()) {
                auto &indexAccessor =
                    asset.accessors[primitiveIt->indicesAccessor.value()];
                indices.resize(indexAccessor.count);

                fastgltf::iterateAccessorWithIndex<std::uint16_t>(
                    asset,
                    indexAccessor,
                    [&](std::uint16_t index, std::size_t idx) {
                        indices[idx] = index;
                    });
            }

            auto positionIt = primitiveIt->findAttribute("POSITION");
            auto normalIt   = primitiveIt->findAttribute("NORMAL");
            auto texCoordIt = primitiveIt->findAttribute("TEXCOORD_0");

            assert(positionIt != primitiveIt->attributes.end());
            assert(normalIt != primitiveIt->attributes.end());
            assert(texCoordIt != primitiveIt->attributes.end());

            auto &positionAccessor = asset.accessors[positionIt->accessorIndex];
            auto &normalAccessor   = asset.accessors[normalIt->accessorIndex];
            auto &texCoordAccessor = asset.accessors[texCoordIt->accessorIndex];

            vertices.resize(positionAccessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset,
                positionAccessor,
                [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                    vertices[idx].pos = pos;
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset,
                normalAccessor,
                [&](fastgltf::math::fvec3 normal, std::size_t idx) {
                    vertices[idx].normal = normal;
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                asset,
                texCoordAccessor,
                [&](fastgltf::math::fvec2 texCoord, std::size_t idx) {
                    vertices[idx].texCoord = texCoord;
                });
        }
    }

    return {indices, vertices};
}

auto readShaders(const std::string &filename) -> std::vector<char>
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file.");
    }

    auto fileSize = file.tellg();
    auto buffer   = std::vector<char>(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

auto main(int argc, char *argv[]) -> int
{
    auto gltfPath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "resources/Duck/glTF/Duck.gltf";
        }

        else {
            return std::string{argv[1]};
        }
    }();

    auto shaderPath = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "shaders/shader.spv";
        }

        else {
            return std::string{argv[2]};
        }
    }();

    // init vk::raii::Context necessary for vulkan.hpp RAII functionality
    auto context = vk::raii::Context{};

    // create custom deleter for SDL_Window
    auto SDL_WindowDeleter = [](SDL_Window *window) { SDL_DestroyWindow(window); };

    // init SDL3 and create window
    auto window = [&SDL_WindowDeleter]
        -> std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)> {
        // init SDL3
        if (!SDL_Init(SDL_INIT_VIDEO)) {
            fmt::print(stderr, "SDL_Init Error: {}\n", SDL_GetError());
            return {};
        }

        // Dynamically load Vulkan loader library
        if (!SDL_Vulkan_LoadLibrary(nullptr)) {
            fmt::print(stderr, "SDL_Vulkan_LoadLibrary Error: {}\n", SDL_GetError());
            return {};
        }

        // create Vulkan window
        auto window = SDL_CreateWindow("Vulkan + SDL3", 800, 600, SDL_WINDOW_VULKAN);
        if (!window) {
            fmt::print(stderr, "SDL_CreateWindow Error: {}\n", SDL_GetError());
            return {};
        }

        return std::unique_ptr<SDL_Window, decltype(SDL_WindowDeleter)>(window);
    }();

    if (!window) {
        return EXIT_FAILURE;
    }

#ifndef VK_EXT_DEBUG_REPORT_EXTENSION_NAME
#define VK_EXT_DEBUG_REPORT_EXTENSION_NAME "VK_EXT_debug_report"
#endif

    std::vector<const char *> instance_extensions{
        VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        VK_EXT_SURFACE_MAINTENANCE_1_EXTENSION_NAME,
        VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME};

    {
        Uint32             sdl_extensions_count;
        const char *const *sdl_instance_extensions =
            SDL_Vulkan_GetInstanceExtensions(&sdl_extensions_count);

        if (sdl_instance_extensions == nullptr) {
            fmt::print(
                stderr,
                "SDL_Vulkan_GetInstanceExtensions Error: {}\n",
                SDL_GetError());
            return -1;
        }
        instance_extensions.insert(
            instance_extensions.end(),
            sdl_instance_extensions,
            sdl_instance_extensions + sdl_extensions_count);
    }

    // --- Create Vulkan instance using vk-bootstrap
    vkb::InstanceBuilder builder;
    auto                 inst_ret = builder.set_app_name("Vulkan SDL3 App")
                        .require_api_version(1, 4)
                        .use_default_debug_messenger()
                        .enable_validation_layers(true)
                        .enable_extensions(instance_extensions)
                        .build();

    if (!inst_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap instance creation failed: {}\n",
            inst_ret.error().message());
        return -1;
    }

    auto vkb_inst = vkb::Instance{inst_ret.value()};
    auto instance = vk::raii::Instance{context, vkb_inst.instance};
    auto debugUtils =
        vk::raii::DebugUtilsMessengerEXT{instance, vkb_inst.debug_messenger};

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    // VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    auto surface = [&] -> vk::raii::SurfaceKHR {
        VkSurfaceKHR surface_vk;
        if (!SDL_Vulkan_CreateSurface(window.get(), *instance, nullptr, &surface_vk)) {
            fmt::print(stderr, "SDL_Vulkan_CreateSurface failed: {}\n", SDL_GetError());
            return nullptr;
        }
        return {instance, surface_vk};
    }();

    // --- Pick physical device and create logical device using vk-bootstrap
    vkb::PhysicalDeviceSelector selector{vkb_inst};

    auto phys_ret =
        selector.set_surface(*surface)
            .add_required_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
            .add_required_extension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)
            .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
            .require_present()
            .select();

    if (!phys_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to select physical device: {}\n",
            phys_ret.error().message());
        return -1;
    }

    auto vkb_phys       = phys_ret.value();
    auto physicalDevice = vk::raii::PhysicalDevice(instance, vkb_phys.physical_device);

    auto dev_builder = vkb::DeviceBuilder{vkb_phys};
    auto dev_ret     = dev_builder.build();
    if (!dev_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create logical device: {}\n",
            dev_ret.error().message());
        return -1;
    }

    auto vkb_device = dev_ret.value();
    auto device     = vk::raii::Device{physicalDevice, vkb_device.device};

    auto graphicsQueue = vkb_device.get_queue(vkb::QueueType::graphics).value();
    auto graphicsQueueIndex =
        vkb_device.get_queue_index(vkb::QueueType::graphics).value();

    VmaVulkanFunctions vulkanFunctions    = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice         = *physicalDevice;
    allocatorCreateInfo.device                 = *device;
    allocatorCreateInfo.instance               = *instance;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                              | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
        fmt::print(stderr, "Failed to create VMA allocator\n");
        return -1;
    }

    auto swap_builder = vkb::SwapchainBuilder{vkb_device};
    auto swap_ret =
        swap_builder
            .set_image_usage_flags(
                VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT)
            .set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_IMMEDIATE_KHR)
            .add_fallback_present_mode(VK_PRESENT_MODE_FIFO_KHR)
            .set_desired_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_B8G8R8A8_UNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .add_fallback_format(
                VkSurfaceFormatKHR{
                    .format     = VK_FORMAT_R8G8B8A8_SNORM,
                    .colorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR})
            .set_desired_min_image_count(3)
            .build();

    if (!swap_ret) {
        fmt::print(
            stderr,
            "vk-bootstrap failed to create swapchain: {}",
            swap_ret.error().message());
        return -1;
    }

    auto vkb_swapchain = swap_ret.value();
    auto swapchain     = vk::raii::SwapchainKHR(device, vkb_swapchain.swapchain);

    fmt::print(stderr, "swapchain image count: {}\n", vkb_swapchain.image_count);

    // get swapchain images

    auto images         = swapchain.getImages();
    auto imageViews     = std::vector<vk::raii::ImageView>{};
    auto imageAvailable = std::vector<vk::raii::Semaphore>{};
    auto renderFinished = std::vector<vk::raii::Semaphore>{};

    for (auto image : images) {
        imageViews.emplace_back(device, vk::ImageViewCreateInfo{{}, image});
        imageAvailable.emplace_back(device, vk::SemaphoreCreateInfo{});
        renderFinished.emplace_back(device, vk::SemaphoreCreateInfo{});
    }

    auto maxFramesInFlight = images.size();

    // create transient command pool for single-time commands
    auto transientCommandPool = vk::raii::CommandPool{
        device,
        vk::CommandPoolCreateInfo{
            vk::CommandPoolCreateFlagBits::eTransient,
            graphicsQueueIndex}};

    auto commandBuffer = beginSingleTimeCommands(device, transientCommandPool);

    endSingleTimeCommands(device, transientCommandPool, commandBuffer, vk::Queue{});
    // Transition image layout
    auto shader_spv = readShaders(shaderPath);

    spv_reflect::ShaderModule reflection(shader_spv.size(), shader_spv.data());

    if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS) {
        fmt::print(
            stderr,
            "ERROR: could not process '{}' (is it a valid SPIR-V bytecode?)\n",
            argv[2]);
        return EXIT_FAILURE;
    }

    auto &shaderModule = reflection.GetShaderModule();

    const auto entry_point_name = std::string{shaderModule.entry_point_name};

    auto result = SPV_REFLECT_RESULT_NOT_READY;
    auto count  = uint32_t{0};

    auto variables = std::vector<SpvReflectInterfaceVariable *>{};
    auto bindings  = std::vector<SpvReflectDescriptorBinding *>{};
    auto sets      = std::vector<SpvReflectDescriptorSet *>{};

    result = reflection.EnumerateDescriptorBindings(&count, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    bindings.resize(count);

    result = reflection.EnumerateDescriptorBindings(&count, bindings.data());
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    auto ToStringDescriptorType = [](SpvReflectDescriptorType value) -> std::string {
        switch (value) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            return "VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return "VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return "VK_DESCRIPTOR_TYPE_STORAGE_IMAGE";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER";
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            return "VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
        case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return "VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
        case SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
        }
        // unhandled SpvReflectDescriptorType enum value
        return "VK_DESCRIPTOR_TYPE_???";
    };

    auto ToStringResourceType = [](SpvReflectResourceType res_type) -> std::string {
        switch (res_type) {
        case SPV_REFLECT_RESOURCE_FLAG_UNDEFINED:
            return "UNDEFINED";
        case SPV_REFLECT_RESOURCE_FLAG_SAMPLER:
            return "SAMPLER";
        case SPV_REFLECT_RESOURCE_FLAG_CBV:
            return "CBV";
        case SPV_REFLECT_RESOURCE_FLAG_SRV:
            return "SRV";
        case SPV_REFLECT_RESOURCE_FLAG_UAV:
            return "UAV";
        }
        // unhandled SpvReflectResourceType enum value
        return "???";
    };

    for (auto p_binding : bindings) {
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        auto binding_name    = p_binding->name;
        auto descriptor_type = ToStringDescriptorType(p_binding->descriptor_type);
        auto resource_type   = ToStringResourceType(p_binding->resource_type);
        if ((p_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
            || (p_binding->descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE)
            || (p_binding->descriptor_type
                == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER)
            || (p_binding->descriptor_type
                == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER)) {
            auto dim     = p_binding->image.dim;
            auto depth   = p_binding->image.depth;
            auto arrayed = p_binding->image.arrayed;
            auto ms      = p_binding->image.ms;
            auto sampled = p_binding->image.sampled;
        }
    }

    const auto descriptorSetLayoutBindings = std::array{
        vk::DescriptorSetLayoutBinding{
            0,
            vk::DescriptorType::eUniformBuffer,
            1,
            vk::ShaderStageFlagBits::eVertex},
        vk::DescriptorSetLayoutBinding{
            1,
            vk::DescriptorType::eCombinedImageSampler,
            1,
            vk::ShaderStageFlagBits::eFragment}};

    auto descriptorSetLayout =
        vk::raii::DescriptorSetLayout{device, vk::DescriptorSetLayoutCreateInfo{}};

    // The stages used by this pipeline
    const auto shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            {},
            {},
            {}},
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            {},
            {},
            {}},
    };

    const auto vertexBindingDescriptions =
        std::array{vk::VertexInputBindingDescription{0, sizeof(Vertex)}};

    const auto vertexAttributeDescriptions = std::array{
        vk::VertexInputAttributeDescription{
            0,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, pos)},
        vk::VertexInputAttributeDescription{
            1,
            0,
            vk::Format::eR32G32B32Sfloat,
            offsetof(Vertex, normal)},
        vk::VertexInputAttributeDescription{
            2,
            0,
            vk::Format::eR32G32Sfloat,
            offsetof(Vertex, texCoord)}};

    const auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        vk::Bool32{false}};

    const auto dynamicStates = std::array{
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount};

    const auto rasterizerStateCreateInfo = vk::PipelineRasterizationStateCreateInfo{
        {},
        {},
        {},
        vk::PolygonMode::eFill,
        vk::CullModeFlags::BitsType::eNone,
        vk::FrontFace::eCounterClockwise,
        {},
        {},
        {},
        {},
        1.0f};

    const auto colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB};

    const auto pipelineColorBlendStateCreateInfo =
        vk::PipelineColorBlendStateCreateInfo{
            {},
            {},
            vk::LogicOp::eCopy,
            1,
            &colorBlendAttachmentState};

    auto pipelineLayout = vk::raii::PipelineLayout{device, {{}, *descriptorSetLayout}};

    auto asset = getGltfAsset(gltfPath);

    auto [indices, vertices] = getGltfAssetData(asset.get());

    // std::vector<vk::raii::DescriptorSetLayout>  descriptorSetLayouts;
    // std::vector<vk::PushConstantRange>          pushConstantRanges;
    // std::vector<vk::DescriptorSetLayoutBinding> descriptorRanges;

    // vk::PipelineLayoutCreateInfo pipelineLayoutInfo;

    // {
    //     auto poolSizes               = std::array<vk::DescriptorPoolSize, 2>{};
    //     poolSizes[0].type            = vk::DescriptorType::eUniformBuffer;
    //     poolSizes[0].descriptorCount =
    //     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); poolSizes[1].type =
    //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; poolSizes[1].descriptorCount =
    //     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    //
    //     auto descriptorPoolInfo  = VkDescriptorPoolCreateInfo{};
    //     descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    //     descriptorPoolInfo.poolSizeCount =
    //     static_cast<uint32_t>(poolSizes.size()); descriptorPoolInfo.pPoolSizes =
    //     poolSizes.data(); descriptorPoolInfo.maxSets       =
    //     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); descriptorPoolInfo.flags =
    //     0;
    //
    //     auto descriptorPool = VkDescriptorPool{};
    //     if (vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr,
    //                                &descriptorPool)
    //         != VK_SUCCESS) {
    //         throw std::runtime_error("failed to create descriptor pool.");
    //     }
    //
    //     return descriptorPool;
    // }
    // ();
    //
    // auto descriptorSetLayout = [logicalDevice] -> VkDescriptorSetLayout {
    //     auto uboLayoutBinding               = VkDescriptorSetLayoutBinding{};
    //     uboLayoutBinding.binding            = 0;
    //     uboLayoutBinding.descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    //     uboLayoutBinding.descriptorCount    = 1;
    //     uboLayoutBinding.pImmutableSamplers = nullptr;
    //     uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;
    //
    //     auto samplerLayoutBinding            = VkDescriptorSetLayoutBinding{};
    //     samplerLayoutBinding.binding         = 1;
    //     samplerLayoutBinding.descriptorCount = 1;
    //     samplerLayoutBinding.descriptorType =
    //     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    //     samplerLayoutBinding.pImmutableSamplers = nullptr;
    //     samplerLayoutBinding.stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT;
    //
    //     auto bindings = std::vector{uboLayoutBinding, samplerLayoutBinding};
    //
    //     auto layoutInfo         = VkDescriptorSetLayoutCreateInfo{};
    //     layoutInfo.sType        =
    //     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    //     layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    //     layoutInfo.pBindings    = bindings.data();
    //
    //     auto descriptorSetLayout = VkDescriptorSetLayout{};
    //     if (vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr,
    //                                     &descriptorSetLayout)
    //         != VK_SUCCESS) {
    //         throw std::runtime_error("failed to create descriptor set layout.");
    //     }
    //
    //     return descriptorSetLayout;
    // }();
    //
    // auto descriptorSets = [logicalDevice, descriptorPool,
    //                        descriptorSetLayout] -> std::vector<VkDescriptorSet> {
    //     auto layouts       = std::vector(MAX_FRAMES_IN_FLIGHT,
    //     descriptorSetLayout); auto allocateInfo  = VkDescriptorSetAllocateInfo{};
    //     allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    //     allocateInfo.descriptorPool     = descriptorPool;
    //     allocateInfo.descriptorSetCount =
    //     static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT); allocateInfo.pSetLayouts =
    //     layouts.data();
    //
    //     auto descriptorSets = std::vector<VkDescriptorSet>{};
    //     descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    //     if (vkAllocateDescriptorSets(logicalDevice, &allocateInfo,
    //                                  descriptorSets.data())
    //         != VK_SUCCESS) {
    //         throw std::runtime_error("failed to allocate descriptors.");
    //     }
    //
    //     return descriptorSets;
    // }();
    //
    // --- Main loop (minimal)
    bool      running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }
        SDL_Delay(16); // Simulate frame
    }

    // --- Cleanup
    vmaDestroyAllocator(allocator);
    SDL_Quit();
    return 0;
}
