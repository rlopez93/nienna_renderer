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

#include <VkBootstrap.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL_vulkan.h>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>

auto main(int argc, char *argv[]) -> int
{
    if (argc < 2) {
        std::cerr << "Oof ouchie, need a gltf file, ouchie owie\n";
        return EXIT_FAILURE;
    }

    // --- SDL3 Init
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << "\n";
        return -1;
    }

    // --- Create Window with Vulkan flag
    SDL_Window *window = SDL_CreateWindow("Vulkan + SDL3", 800, 600, SDL_WINDOW_VULKAN);
    if (!window) {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << "\n";
        return -1;
    }

    // --- Load Vulkan library
    if (!SDL_Vulkan_LoadLibrary(nullptr)) {
        std::cerr << "SDL_Vulkan_LoadLibrary Error: " << SDL_GetError() << "\n";
        return -1;
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
            std::cerr << "SDL_Vulkan_GetInstanceExtensions failed:" << SDL_GetError()
                      << "\n";
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
        std::cerr << "vk-bootstrap instance creation failed: "
                  << inst_ret.error().message() << "\n";
        return -1;
    }

    vkb::Instance vkb_inst = inst_ret.value();
    vk::Instance  instance = vkb_inst.instance;

    VULKAN_HPP_DEFAULT_DISPATCHER.init();
    VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

    auto surface = [&] -> vk::SurfaceKHR {
        VkSurfaceKHR surface_vk;
        if (!SDL_Vulkan_CreateSurface(window, instance, nullptr, &surface_vk)) {
            std::cerr << "SDL_Vulkan_CreateSurface failed\n";
            return nullptr;
        }
        return surface_vk;
    }();

    // --- Pick physical device and create logical device using vk-bootstrap
    vkb::PhysicalDeviceSelector selector{vkb_inst};

    auto phys_ret =
        selector.set_surface(surface)
            .add_required_extension(VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_2_EXTENSION_NAME)
            .add_required_extension(VK_EXT_EXTENDED_DYNAMIC_STATE_3_EXTENSION_NAME)
            .add_required_extension(VK_EXT_SWAPCHAIN_MAINTENANCE_1_EXTENSION_NAME)
            .add_required_extension(VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME)
            .require_present()
            .select();

    if (!phys_ret) {
        std::cerr << "Failed to select physical device: " << phys_ret.error().message()
                  << "\n";
        return -1;
    }

    vkb::PhysicalDevice vkb_phys = phys_ret.value();

    vkb::DeviceBuilder dev_builder{vkb_phys};
    auto               dev_ret = dev_builder.build();
    if (!dev_ret) {
        std::cerr << "Failed to create logical device: " << dev_ret.error().message()
                  << "\n";
        return -1;
    }

    vkb::Device        vkb_device     = dev_ret.value();
    vk::Device         device         = vkb_device.device;
    vk::PhysicalDevice physicalDevice = vkb_phys.physical_device;

    VmaVulkanFunctions vulkanFunctions    = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice         = physicalDevice;
    allocatorCreateInfo.device                 = device;
    allocatorCreateInfo.instance               = instance;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                              | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
        std::cerr << "Failed to create VMA allocator\n";
        return -1;
    }

    vkb::SwapchainBuilder swap_builder{vkb_device};
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

    static constexpr auto supportedExtensions =
        fastgltf::Extensions::KHR_mesh_quantization
        | fastgltf::Extensions::KHR_texture_transform
        | fastgltf::Extensions::KHR_materials_variants;

    fastgltf::Parser parser(supportedExtensions);

    constexpr auto gltfOptions =
        fastgltf::Options::DontRequireValidAssetMember | fastgltf::Options::AllowDouble
        | fastgltf::Options::LoadExternalBuffers | fastgltf::Options::LoadExternalImages
        | fastgltf::Options::GenerateMeshIndices;

    auto path = std::filesystem::path(argv[1]);

    auto gltfFile = fastgltf::GltfDataBuffer::FromPath(path);
    if (!bool(gltfFile)) {
        std::cerr << "Failed to open glTF file: "
                  << fastgltf::getErrorMessage(gltfFile.error()) << '\n';
        return EXIT_FAILURE;
    }

    auto asset = parser.loadGltf(gltfFile.get(), path.parent_path(), gltfOptions);

    if (fastgltf::getErrorName(asset.error()) != "None") {
        std::cerr << "Failed to load glTF: " << fastgltf::getErrorMessage(asset.error())
                  << '\n';
        return EXIT_FAILURE;
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

    auto descriptorSetLayout = device.createDescriptorSetLayout(
        vk::DescriptorSetLayoutCreateInfo{{}, descriptorSetLayoutBindings});

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

    struct Vertex {
        fastgltf::math::fvec3 pos{};
        fastgltf::math::fvec3 normal{};
        fastgltf::math::fvec2 texCoord{};
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

    auto pipelineLayout = device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo{{}, descriptorSetLayout});

    std::vector<std::uint16_t> indices;
    std::vector<Vertex>        vertices;

    for (auto &mesh : asset->meshes) {
        std::cerr << "Mesh is: <" << mesh.name << ">\n";
        for (auto primitiveIt = mesh.primitives.begin();
             primitiveIt != mesh.primitives.end();
             ++primitiveIt) {

            if (primitiveIt->indicesAccessor.has_value()) {
                auto &indexAccessor =
                    asset->accessors[primitiveIt->indicesAccessor.value()];
                indices.resize(indexAccessor.count);

                fastgltf::iterateAccessorWithIndex<std::uint16_t>(
                    asset.get(),
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

            auto &positionAccessor = asset->accessors[positionIt->accessorIndex];
            auto &normalAccessor   = asset->accessors[normalIt->accessorIndex];
            auto &texCoordAccessor = asset->accessors[texCoordIt->accessorIndex];

            vertices.resize(positionAccessor.count);

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset.get(),
                positionAccessor,
                [&](fastgltf::math::fvec3 pos, std::size_t idx) {
                    vertices[idx].pos = pos;
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(
                asset.get(),
                normalAccessor,
                [&](fastgltf::math::fvec3 normal, std::size_t idx) {
                    vertices[idx].normal = normal;
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(
                asset.get(),
                texCoordAccessor,
                [&](fastgltf::math::fvec2 texCoord, std::size_t idx) {
                    vertices[idx].texCoord = texCoord;
                });
        }

        break;
    }

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
    vkDestroySurfaceKHR(instance, surface, nullptr);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
