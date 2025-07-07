#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_extension_inspection.hpp>
#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
#include <numeric>
#include <ranges>
#include <variant>

#include "Renderer.hpp"

struct Vertex;
struct VertexReflectionData;

auto readShaders(const std::string &filename) -> std::vector<char>;
auto reflectShader(const std::filesystem::path &path) -> VertexReflectionData;

auto getGltfAsset(const std::filesystem::path &gltfPath)
    -> fastgltf::Expected<fastgltf::Asset>;

auto getGltfAssetData(fastgltf::Asset &asset) -> std::pair<
    std::vector<uint16_t>,
    std::vector<Vertex>>;

void cmdTransitionImageLayout(
    vk::raii::CommandBuffer &cmd,
    VkImage                  image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout,
    vk::ImageAspectFlags     aspectMask);

auto makePipelineStageAccessTuple(vk::ImageLayout state) -> std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2>;

auto makePipelineStageAccessTuple(vk::ImageLayout state) -> std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2>
{
    switch (state) {
    case vk::ImageLayout::eUndefined:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::AccessFlagBits2::eNone);
    case vk::ImageLayout::eColorAttachmentOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentRead
                | vk::AccessFlagBits2::eColorAttachmentWrite);
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eFragmentShader
                | vk::PipelineStageFlagBits2::eComputeShader
                | vk::PipelineStageFlagBits2::ePreRasterizationShaders,
            vk::AccessFlagBits2::eShaderRead);
    case vk::ImageLayout::eTransferDstOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite);
    case vk::ImageLayout::eGeneral:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eComputeShader
                | vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite
                | vk::AccessFlagBits2::eTransferWrite);
    case vk::ImageLayout::ePresentSrcKHR:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone);
    default: {
        assert(false && "Unsupported layout transition!");
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eAllCommands,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
    }
    }
}

void cmdTransitionImageLayout(
    vk::raii::CommandBuffer &cmd,
    VkImage                  image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout,
    vk::ImageAspectFlags     aspectMask)

{
    const auto [srcStage, srcAccess] = makePipelineStageAccessTuple(oldLayout);
    const auto [dstStage, dstAccess] = makePipelineStageAccessTuple(newLayout);

    auto barrier = vk::ImageMemoryBarrier2{
        srcStage,
        srcAccess,
        dstStage,
        dstAccess,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        {aspectMask, 0, 1, 0, 1}};

    cmd.pipelineBarrier2(vk::DependencyInfo{{}, {}, {}, barrier});
}

auto beginSingleTimeCommands(
    vk::raii::Device      &device,
    vk::raii::CommandPool &commandPool) -> vk::raii::CommandBuffer;

auto endSingleTimeCommands(
    vk::raii::Device      &device,
    vk::raii::CommandPool &commandPool,
    vk::CommandBuffer     &commandBuffer,
    vk::Queue             &queue);

auto endSingleTimeCommands(
    vk::raii::Device        &device,
    vk::raii::CommandPool   &commandPool,
    vk::raii::CommandBuffer &commandBuffer,
    vk::raii::Queue         &queue)
{
    // submit command buffer
    commandBuffer.end();

    // create fence for synchronization
    auto fenceCreateInfo         = vk::FenceCreateInfo{};
    auto fence                   = vk::raii::Fence{device, fenceCreateInfo};
    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{commandBuffer};

    queue.submit2(vk::SubmitInfo2{{}, {}, commandBufferSubmitInfo}, fence);
    auto result =
        device.waitForFences(*fence, true, std::numeric_limits<uint64_t>::max());

    assert(result == vk::Result::eSuccess);
}

auto beginSingleTimeCommands(
    vk::raii::Device      &device,
    vk::raii::CommandPool &commandPool) -> vk::raii::CommandBuffer
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

    constexpr auto gltfOptions = fastgltf::Options::DontRequireValidAssetMember
                               | fastgltf::Options::AllowDouble
                               | fastgltf::Options::LoadExternalBuffers
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

auto getGltfAssetData(fastgltf::Asset &asset) -> std::pair<
    std::vector<uint16_t>,
    std::vector<Vertex>>
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

            assert(primitiveIt->materialIndex.has_value());

            fastgltf::Material &material =
                asset.materials[primitiveIt->materialIndex.value()];
            assert(material.pbrData.baseColorTexture.has_value());

            fastgltf::TextureInfo &textureInfo =
                material.pbrData.baseColorTexture.value();
            assert(textureInfo.texCoordIndex == 0);

            fastgltf::Texture &texture = asset.textures[textureInfo.textureIndex];
            assert(texture.imageIndex.has_value());

            fastgltf::Image &image = asset.images[texture.imageIndex.value()];

            fastgltf::URI &imageURI = std::get<fastgltf::sources::URI>(image.data).uri;

            auto image_data = std::vector<unsigned char>{};
            {
                int  x, y, n;
                auto stbi_data = stbi_load(imageURI.c_str(), &x, &y, &n, 0);

                image_data.assign(stbi_data, stbi_data + (x * y));

                stbi_image_free(stbi_data);

                // MapMemory -> copy to GPU -> UnmapMemory
            }
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

struct VertexReflectionData {

    std::vector<SpvReflectInterfaceVariable *> variables;
    std::vector<SpvReflectDescriptorBinding *> bindings;
    std::vector<SpvReflectDescriptorSet *>     sets;
};

auto reflectShader(const std::vector<char> &code) -> VertexReflectionData
{
    auto reflection = spv_reflect::ShaderModule(code.size(), code.data());

    if (reflection.GetResult() != SPV_REFLECT_RESULT_SUCCESS) {
        fmt::print(
            stderr,
            "ERROR: could not process shader code (is it a valid SPIR-V bytecode?)\n");
        throw std::exception{};
    }

    auto &shaderModule = reflection.GetShaderModule();

    const auto entry_point_name = std::string{shaderModule.entry_point_name};

    fmt::print(stderr, "{}\n", entry_point_name);

    auto result = SPV_REFLECT_RESULT_NOT_READY;
    auto count  = uint32_t{0};

    auto data = VertexReflectionData{};

    result = reflection.EnumerateDescriptorBindings(&count, nullptr);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);
    data.bindings.resize(count);

    result = reflection.EnumerateDescriptorBindings(&count, data.bindings.data());
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

    for (auto p_binding : data.bindings) {
        assert(result == SPV_REFLECT_RESULT_SUCCESS);
        auto binding_name    = p_binding->name;
        auto descriptor_type = ToStringDescriptorType(p_binding->descriptor_type);
        auto resource_type   = ToStringResourceType(p_binding->resource_type);

        fmt::print(stderr, "{} {} {}\n", binding_name, descriptor_type, resource_type);
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

            fmt::print(
                stderr,
                "dim {} depth {} arrayed {} ms {} sampled {}\n",
                (int)dim,
                depth,
                arrayed,
                ms,
                sampled);
        }
    }

    return data;
}

auto createPipeline(
    vk::raii::Device         &device,
    vk::Format                imageFormat,
    vk::Format                depthFormat,
    vk::raii::ShaderModule   &vertShaderModule,
    vk::raii::ShaderModule   &fragShaderModule,
    vk::raii::PipelineLayout &pipelineLayout) -> vk::raii::Pipeline
{
    // The stages used by this pipeline
    const auto shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            vertShaderModule,
            "main",
            {}},
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            fragShaderModule,
            "main",
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

    const auto vertexInputStateCreateInfo = vk::PipelineVertexInputStateCreateInfo{
        {},
        vertexBindingDescriptions,
        vertexAttributeDescriptions};

    const auto inputAssemblyCreateInfo = vk::PipelineInputAssemblyStateCreateInfo{
        {},
        vk::PrimitiveTopology::eTriangleList,
        false};

    const auto pipelineViewportStateCreateInfo = vk::PipelineViewportStateCreateInfo{};

    const auto pipelineRasterizationStateCreateInfo =
        vk::PipelineRasterizationStateCreateInfo{
            {},
            {},
            {},
            vk::PolygonMode::eFill,
            vk::CullModeFlags::BitsType::eBack,
            vk::FrontFace::eCounterClockwise,
            {},
            {},
            {},
            {},
            1.0f};

    const auto pipelineMultisampleStateCreateInfo =
        vk::PipelineMultisampleStateCreateInfo{};

    const auto colorBlendAttachmentState = vk::PipelineColorBlendAttachmentState{
        {},
        {},
        {},
        {},
        {},
        {},
        {},
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG
            | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA};

    const auto pipelineColorBlendStateCreateInfo =
        vk::PipelineColorBlendStateCreateInfo{
            {},
            {},
            vk::LogicOp::eCopy,
            1,
            &colorBlendAttachmentState};

    const auto dynamicStates = std::array{
        vk::DynamicState::eViewportWithCount,
        vk::DynamicState::eScissorWithCount};

    const auto pipelineDynamicStateCreateInfo =
        vk::PipelineDynamicStateCreateInfo{{}, dynamicStates};

    auto pipelineRenderingCreateInfo =
        vk::PipelineRenderingCreateInfo{{}, imageFormat, depthFormat};

    auto graphicsPipelineCreateInfo = vk::GraphicsPipelineCreateInfo{
        {},
        shaderStages,
        &vertexInputStateCreateInfo,
        &inputAssemblyCreateInfo,
        {},
        &pipelineViewportStateCreateInfo,
        &pipelineRasterizationStateCreateInfo,
        &pipelineMultisampleStateCreateInfo,
        {},
        &pipelineColorBlendStateCreateInfo,
        &pipelineDynamicStateCreateInfo,
        *pipelineLayout,
        {},
        {},
        {},
        {},
        &pipelineRenderingCreateInfo};

    return vk::raii::Pipeline{device, nullptr, graphicsPipelineCreateInfo};
}

auto createShaderModules(vk::raii::Device &device) -> std::tuple<
    vk::raii::ShaderModule,
    vk::raii::ShaderModule>
{

    auto vertShaderCode = readShaders("shaders/shader.vert.spv");
    auto fragShaderCode = readShaders("shaders/shader.frag.spv");

    // auto vertexData = reflectShader(vertShaderCode);

    auto vertShaderModule = vk::raii::ShaderModule{
        device,
        vk::ShaderModuleCreateInfo{
            {},
            vertShaderCode.size(),
            reinterpret_cast<const uint32_t *>(vertShaderCode.data())}};

    auto fragShaderModule = vk::raii::ShaderModule{
        device,
        vk::ShaderModuleCreateInfo{
            {},
            fragShaderCode.size(),
            reinterpret_cast<const uint32_t *>(fragShaderCode.data())}};

    return {std::move(vertShaderModule), std::move(fragShaderModule)};
}

auto createVmaAllocator(RenderContext &ctx) -> VmaAllocator
{

    VmaVulkanFunctions vulkanFunctions    = {};
    vulkanFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCreateInfo = {};
    allocatorCreateInfo.physicalDevice         = *ctx.physicalDevice;
    allocatorCreateInfo.device                 = *ctx.device;
    allocatorCreateInfo.instance               = *ctx.instance;
    allocatorCreateInfo.vulkanApiVersion       = VK_API_VERSION_1_4;
    allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT
                              | VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT;
    allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

    VmaAllocator allocator;
    if (vmaCreateAllocator(&allocatorCreateInfo, &allocator) != VK_SUCCESS) {
        fmt::print(stderr, "Failed to create VMA allocator\n");
        throw std::exception{};
    }

    return allocator;
}

auto main(
    int    argc,
    char **argv) -> int
{
    auto gltfPath = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "resources/Duck/glTF/Duck.gltf";
        } else {
            return std::string{argv[1]};
        }
    }();

    auto shaderPath = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "shaders/shader.vert.spv";
        } else {
            return std::string{argv[2]};
        }
    }();

    Renderer     r;
    VmaAllocator vmaAllocator = createVmaAllocator(r.ctx);

    // get swapchain images

    auto images            = r.ctx.swapchain.getImages();
    auto maxFramesInFlight = images.size();

    auto imageViews     = std::vector<vk::raii::ImageView>{};
    auto imageAvailable = std::vector<vk::raii::Semaphore>{};
    auto renderFinished = std::vector<vk::raii::Semaphore>{};

    for (auto image : images) {
        imageViews.emplace_back(
            r.ctx.device,
            vk::ImageViewCreateInfo{
                {},
                image,
                vk::ImageViewType::e2D,
                r.ctx.imageFormat,
                {},
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
        imageAvailable.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
        renderFinished.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
    }

    // create transient command pool for single-time commands
    auto transientCommandPool = vk::raii::CommandPool{
        r.ctx.device,
        {vk::CommandPoolCreateFlagBits::eTransient, r.ctx.graphicsQueueIndex}};

    // Transition image layout
    auto commandBuffer = beginSingleTimeCommands(r.ctx.device, transientCommandPool);

    for (auto image : images) {
        cmdTransitionImageLayout(
            commandBuffer,
            image,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::ePresentSrcKHR,
            vk::ImageAspectFlagBits::eColor);
    }

    endSingleTimeCommands(
        r.ctx.device,
        transientCommandPool,
        commandBuffer,
        r.ctx.graphicsQueue);

    auto commandPools   = std::vector<vk::raii::CommandPool>{};
    auto commandBuffers = std::vector<vk::raii::CommandBuffer>{};
    auto frameNumbers   = std::vector<uint64_t>{maxFramesInFlight, 0};
    std::ranges::iota(frameNumbers, 0);

    uint64_t initialValue = maxFramesInFlight - 1;

    auto timelineSemaphoreCreateInfo =
        vk::SemaphoreTypeCreateInfo{vk::SemaphoreType::eTimeline, initialValue};
    auto frameTimelineSemaphore =
        vk::raii::Semaphore{r.ctx.device, {{}, &timelineSemaphoreCreateInfo}};

    for (auto n : std::views::iota(0u, maxFramesInFlight)) {

        commandPools.emplace_back(
            r.ctx.device,
            vk::CommandPoolCreateInfo{{}, r.ctx.graphicsQueueIndex});

        commandBuffers.emplace_back(
            std::move(
                vk::raii::CommandBuffers{
                    r.ctx.device,
                    vk::CommandBufferAllocateInfo{
                        commandPools.back(),
                        vk::CommandBufferLevel::ePrimary,
                        1}}
                    .front()));
    }

    // create descriptor pool for ImGui (?)

    // init ImGui

    // create GBuffer

    auto asset = getGltfAsset(gltfPath);

    auto [indices, vertices] = getGltfAssetData(asset.get());

    auto [vertShaderModule, fragShaderModule] = createShaderModules(r.ctx.device);

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
        vk::raii::DescriptorSetLayout{r.ctx.device, {{}, descriptorSetLayoutBindings}};

    auto pipelineLayout =
        vk::raii::PipelineLayout{r.ctx.device, {{}, *descriptorSetLayout}};

    auto graphicsPipeline = createPipeline(
        r.ctx.device,
        r.ctx.imageFormat,
        r.ctx.depthFormat,
        vertShaderModule,
        fragShaderModule,
        pipelineLayout);

    // auto renderingAttachmentInfo =
    //     vk::RenderingAttachmentInfo{{}, {}, {}, {}, {}, {}, {}, {}};

    uint32_t frameRingCurrent = 0;
    uint32_t currentFrame     = 0;

    bool swapchainNeedRebuild = false;

    bool      running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // check swapchain rebuild
        if (swapchainNeedRebuild) {
        }

        // get current frame data
        auto &cmdPool     = commandPools[frameRingCurrent];
        auto &cmd         = commandBuffers[frameRingCurrent];
        auto  frameNumber = frameNumbers[frameRingCurrent];

        {
            // wait semaphore frame (n - numFrames)
            auto res = r.ctx.device.waitSemaphores(
                vk::SemaphoreWaitInfo{{}, frameTimelineSemaphore, frameNumber},
                std::numeric_limits<uint64_t>::max());
        }

        cmdPool.reset();
        cmd.begin(
            vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

        auto [result, nextImageIndex] = r.ctx.swapchain.acquireNextImage(
            std::numeric_limits<uint64_t>::max(),
            imageAvailable[currentFrame]);

        if (result == vk::Result::eErrorOutOfDateKHR) {
            swapchainNeedRebuild = true;
        } else if (!(result == vk::Result::eSuccess
                     || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        if (swapchainNeedRebuild) {
        }

        // update uniform buffers

        // get color attachment image to render to: vk::RenderingAttachmentInfo

        // get depth attachment buffer: vk::RenderingAttachmentInfo

        // vk::RenderingInfo

        // transition swapchain image layout:
        // vk::ImageLayout::eGeneral -> vk::ImageLayout::eColorAttachmentOptimal

        cmd.beginRendering(vk::RenderingInfo{});

        cmd.setViewportWithCount({/*viewport*/});
        cmd.setScissorWithCount({/*scissor*/});

        // bind texture resources passed to shader
        // cmd.bindDescriptorSets2(vk::BindDescriptorSetsInfo{});

        // bind vertex data
        // cmd.bindVertexBuffers(uint32_t firstBinding, const vk::ArrayProxy<const
        // vk::Buffer> &buffers, const vk::ArrayProxy<const vk::DeviceSize> &offsets);

        // bind index data
        // cmd.bindIndexBuffer(vk::Buffer buffer, vk::DeviceSize offset, vk::IndexType
        // indexType)

        // cmd.drawIndexed(uint32_t indexCount, uint32_t instanceCount,
        // uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)

        cmd.endRendering();

        // transition swapchain image layout: color attach opt -> general

        // beginDynamicRenderingToSwapchain()

        auto colorAttachment = vk::RenderingAttachmentInfo{}
                                   .setImageView(imageViews[nextImageIndex])
                                   .setLoadOp(vk::AttachmentLoadOp::eClear)
                                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                                   .setClearValue({{0.0f, 0.0f, 0.0f, 1.0f}})
                                   .setImageLayout(vk::ImageLayout::eAttachmentOptimal);

        // transition image layout imageViews[nextImageIndex]
        // ePresentSrcKHR -> eColorAttachmentOptimal
        cmd.beginRendering({});

        cmd.endRendering();
        // transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
        // SDL_Delay(16); // Simulate frame
    }

    // --- Cleanup
    vmaDestroyAllocator(vmaAllocator);
    SDL_Quit();
    return 0;
}
