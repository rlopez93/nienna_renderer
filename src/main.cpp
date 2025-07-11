#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_extension_inspection.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <SDL3/SDL_events.h>

#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/math.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include "spirv_reflect.h"
#include "stb_image.h"

#include <fmt/base.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numeric>
#include <ranges>
#include <variant>

#include "Allocator.hpp"
#include "Renderer.hpp"
#include "Utility.hpp"

struct Vertex;
struct VertexReflectionData;

struct Vertex {
    fastgltf::math::fvec3 pos{};
    fastgltf::math::fvec3 normal{};
    fastgltf::math::fvec2 texCoord{};
};

struct Scene {
    fastgltf::math::fmat4x4 model{};
    fastgltf::math::fmat4x4 view{};
    fastgltf::math::fmat4x4 projection{};
};

auto readShaders(const std::string &filename) -> std::vector<char>;
auto reflectShader(const std::filesystem::path &path) -> VertexReflectionData;

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

auto getGltfAsset(const std::filesystem::path &gltfPath)
    -> fastgltf::Expected<fastgltf::Asset>;

auto getGltfAssetData(
    fastgltf::Asset             &asset,
    const std::filesystem::path &path)
    -> std::tuple<
        std::vector<uint16_t>,
        std::vector<Vertex>,
        std::vector<unsigned char>,
        vk::Extent2D,
        fastgltf::math::fmat4x4,
        fastgltf::math::fmat4x4,
        fastgltf::math::fmat4x4,
        vk::SamplerCreateInfo>;

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

auto getGltfAssetData(
    fastgltf::Asset             &asset,
    const std::filesystem::path &directory)
    -> std::tuple<
        std::vector<uint16_t>,
        std::vector<Vertex>,
        std::vector<unsigned char>,
        vk::Extent2D,
        fastgltf::math::fmat4x4,
        fastgltf::math::fmat4x4,
        fastgltf::math::fmat4x4,
        vk::SamplerCreateInfo>
{
    auto indices           = std::vector<uint16_t>{};
    auto vertices          = std::vector<Vertex>{};
    auto image_data        = std::vector<unsigned char>{};
    auto imageExtent       = vk::Extent2D{};
    auto modelMatrix       = fastgltf::math::fmat4x4{};
    auto viewMatrix        = fastgltf::math::fmat4x4{};
    auto projectionMatrix  = fastgltf::math::fmat4x4{};
    auto samplerCreateInfo = vk::SamplerCreateInfo{};

    fastgltf::iterateSceneNodes(
        asset,
        0,
        fastgltf::math::fmat4x4{},
        [&](fastgltf::Node &node, fastgltf::math::fmat4x4 matrix) {
            if (node.cameraIndex.has_value()) {
                auto &camera = asset.cameras[node.cameraIndex.value()];
                viewMatrix   = matrix;
                if (std::holds_alternative<fastgltf::Camera::Perspective>(
                        camera.camera)) {
                    auto cameraProjection =
                        std::get<fastgltf::Camera::Perspective>(camera.camera);

                    auto a = cameraProjection.aspectRatio.value_or(1.0f);
                    auto y = cameraProjection.yfov;
                    auto n = cameraProjection.znear;

                    projectionMatrix = [&] -> fastgltf::math::fmat4x4 {
                        if (cameraProjection.zfar.has_value()) {
                            // finite perspective projection
                            float f = cameraProjection.zfar.value();
                            return fastgltf::math::fmat4x4{
                                1.0f / (a * tan(0.5f * y)),
                                0,
                                0,
                                0,
                                0,
                                1.0f / tan(0.5f * y),
                                0,
                                0,
                                0,
                                0,
                                (f + n) / (n - f),
                                -1,
                                0,
                                0,
                                (2 * f * n) / (n - f),
                                0};
                        }

                        else {
                            // infinite perspective projection
                            return fastgltf::math::fmat4x4{
                                1.0f / (a * tan(0.5f * y)),
                                0,
                                0,
                                0,
                                0,
                                1.0f / tan(0.5f * y),
                                0,
                                0,
                                0,
                                0,
                                -1,
                                -1,
                                0,
                                0,
                                2 * n,
                                0};
                        }
                    }();
                }

                fmt::print(
                    stderr,
                    "matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);

                matrix = fastgltf::getTransformMatrix(node);
                fmt::print(
                    stderr,
                    "local matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} "
                    "{}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);
            }

            else if (node.meshIndex.has_value()) {
                auto &mesh = asset.meshes[node.meshIndex.value()];
                fmt::print(stderr, "Mesh is: <{}>\n", mesh.name);

                modelMatrix = matrix;

                fmt::print(
                    stderr,
                    "matrix is:{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n{} {} {} {}\n",
                    matrix[0][0],
                    matrix[0][1],
                    matrix[0][2],
                    matrix[0][3],
                    matrix[1][0],
                    matrix[1][1],
                    matrix[1][2],
                    matrix[1][3],
                    matrix[2][0],
                    matrix[2][1],
                    matrix[2][2],
                    matrix[2][3],
                    matrix[3][0],
                    matrix[3][1],
                    matrix[3][2],
                    matrix[3][3]);

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
                    fastgltf::Texture &texture =
                        asset.textures[textureInfo.textureIndex];

                    assert(texture.imageIndex.has_value());
                    fastgltf::Image &image = asset.images[texture.imageIndex.value()];

                    fastgltf::URI &imageURI =
                        std::get<fastgltf::sources::URI>(image.data).uri;

                    {
                        auto path = directory / imageURI.fspath();
                        fmt::print(stderr, "\"{}\"\n", path.c_str());
                        int  x, y, n;
                        auto stbi_data = stbi_load(path.c_str(), &x, &y, &n, 4);

                        fmt::print(stderr, "channels in file: {}\n", n);
                        assert(stbi_data);

                        image_data.assign(stbi_data, stbi_data + (x * y * 4));
                        // assert(strcmp(image_data.data(), stbi_data) == 0);
                        imageExtent.setWidth(x).setHeight(y);

                        stbi_image_free(stbi_data);
                    }

                    assert(texture.samplerIndex.has_value());
                    fastgltf::Sampler &textureSampler =
                        asset.samplers[texture.samplerIndex.value()];

                    // TODO make vk::SamplerCreateInfo from fastgltf::Sampler
                }
            }
        });

    return {
        indices,
        vertices,
        image_data,
        imageExtent,
        modelMatrix,
        viewMatrix,
        projectionMatrix,
        samplerCreateInfo};
}

auto createPipeline(
    vk::raii::Device         &device,
    vk::Format                imageFormat,
    vk::Format                depthFormat,
    vk::raii::ShaderModule   &shaderModule,
    vk::raii::PipelineLayout &pipelineLayout) -> vk::raii::Pipeline
{
    // The stages used by this pipeline
    const auto shaderStages = std::array{
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eVertex,
            shaderModule,
            "vertexMain",
            {}},
        vk::PipelineShaderStageCreateInfo{
            {},
            vk::ShaderStageFlagBits::eFragment,
            shaderModule,
            "fragmentMain",
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

    const auto pipelineDepthStencilStateCreateInfo =
        vk::PipelineDepthStencilStateCreateInfo{
            {},
            true,
            true,
            vk::CompareOp::eLessOrEqual};

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
        &pipelineDepthStencilStateCreateInfo,
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

auto createShaderModule(vk::raii::Device &device) -> vk::raii::ShaderModule
{
    auto shaderCode = readShaders("shaders/shader.slang.spv");

    auto data = reflectShader(shaderCode);

    return vk::raii::ShaderModule{
        device,
        vk::ShaderModuleCreateInfo{
            {},
            shaderCode.size(),
            reinterpret_cast<const uint32_t *>(shaderCode.data())}};
}

auto uploadBuffers(
    vk::raii::Device                 &device,
    vk::raii::CommandPool            &transientCommandPool,
    const std::vector<Vertex>        &vertexData,
    const std::vector<uint16_t>      &indexData,
    const std::vector<unsigned char> &textureData,
    const vk::Extent2D                textureExtent,
    vk::raii::Queue                  &queue,
    Allocator                        &allocator)
    -> std::tuple<
        Buffer,
        Buffer,
        Image,
        vk::raii::ImageView>
{
    auto commandBuffer = beginSingleTimeCommands(device, transientCommandPool);

    auto vertexBuffer = allocator.createBufferAndUploadData(
        commandBuffer,
        vertexData,
        vk::BufferUsageFlagBits2::eVertexBuffer);
    auto indexBuffer = allocator.createBufferAndUploadData(
        commandBuffer,
        indexData,
        vk::BufferUsageFlagBits2::eIndexBuffer);
    fmt::print(
        stderr,
        "texture extent: {}x{}\n",
        textureExtent.width,
        textureExtent.height);
    auto textureImage = allocator.createImageAndUploadData(
        commandBuffer,
        textureData,
        vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            vk::Format::eR8G8B8A8Srgb,
            // vk::Format::eR8G8B8A8Unorm,
            {static_cast<uint32_t>(textureExtent.width),
             static_cast<uint32_t>(textureExtent.height),
             1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eSampled},
        vk::ImageLayout::eShaderReadOnlyOptimal);
    auto imageView = device.createImageView(
        vk::ImageViewCreateInfo{
            {},
            textureImage.image,
            vk::ImageViewType::e2D,
            vk::Format::eR8G8B8A8Srgb,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
    endSingleTimeCommands(device, transientCommandPool, commandBuffer, queue);

    return {vertexBuffer, indexBuffer, textureImage, std::move(imageView)};
}

auto main(
    int    argc,
    char **argv) -> int
{
    auto gltfDirectory = [&] -> std::filesystem::path {
        if (argc < 2) {
            return "resources/Duck/glTF/";
        } else {
            return std::string{argv[1]};
        }
    }();

    auto gltfFilename = [&] -> std::filesystem::path {
        if (argc < 3) {
            return "Duck.gltf";
        } else {
            return std::string{argv[2]};
        }
    }();

    auto shaderPath = std::filesystem::path("shaders/shader.vert.spv");

    Renderer  r;
    Allocator allocator{
        r.ctx.instance,
        r.ctx.physicalDevice,
        r.ctx.device,
        vk::ApiVersion14};

    // get swapchain images
    auto swapchainImages     = r.ctx.swapchain.getImages();
    auto swapchainImageViews = std::vector<vk::raii::ImageView>{};
    auto maxFramesInFlight   = swapchainImages.size();

    auto imageAvailable = std::vector<vk::raii::Semaphore>{};
    auto renderFinished = std::vector<vk::raii::Semaphore>{};

    for (auto image : swapchainImages) {
        swapchainImageViews.emplace_back(
            r.ctx.device,
            vk::ImageViewCreateInfo{
                {},
                image,
                vk::ImageViewType::e2D,
                r.ctx.imageFormat,
                {},
                {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1}});
    }

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        imageAvailable.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
        renderFinished.emplace_back(r.ctx.device, vk::SemaphoreCreateInfo{});
    }

    auto depthImage = allocator.createImage(
        vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            r.ctx.depthFormat,
            vk::Extent3D(r.ctx.windowExtent, 1),
            1,
            1

        }
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment));

    auto depthImageView = r.ctx.device.createImageView(
        vk::ImageViewCreateInfo{
            {},
            depthImage.image,
            vk::ImageViewType::e2D,
            r.ctx.depthFormat,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});

    // create transient command pool for single-time commands
    auto transientCommandPool = vk::raii::CommandPool{
        r.ctx.device,
        {vk::CommandPoolCreateFlagBits::eTransient, r.ctx.graphicsQueueIndex}};

    // Transition image layout
    auto commandBuffer = beginSingleTimeCommands(r.ctx.device, transientCommandPool);

    for (auto image : swapchainImages) {
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
    auto frameNumbers   = std::vector<uint64_t>(maxFramesInFlight, 0);
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

    auto asset = getGltfAsset(gltfDirectory / gltfFilename);

    auto
        [indexData,
         vertexData,
         textureData,
         textureExtent,
         modelMatrix,
         viewMatrix,
         projectionMatrix,
         samplerCreateInfo] = getGltfAssetData(asset.get(), gltfDirectory);

    auto sampler = vk::raii::Sampler{r.ctx.device, samplerCreateInfo};

    auto [vertexBuffer, indexBuffer, textureImage, textureImageView] = uploadBuffers(
        r.ctx.device,
        transientCommandPool,
        vertexData,
        indexData,
        textureData,
        textureExtent,
        r.ctx.graphicsQueue,
        allocator);

    auto sceneBuffers = std::vector<Buffer>{};

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        sceneBuffers.emplace_back(allocator.createBuffer(
            sizeof(Scene),
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));

        // sceneBuffersMapped ??
    }

    auto shaderModule = createShaderModule(r.ctx.device);

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

    auto descriptorSetLayout = vk::raii::DescriptorSetLayout{
        r.ctx.device,
        vk::DescriptorSetLayoutCreateInfo{{}, descriptorSetLayoutBindings}};

    auto descriptorSetLayouts =
        std::vector<vk::DescriptorSetLayout>(maxFramesInFlight, descriptorSetLayout);

    auto poolSizes = std::array{
        vk::DescriptorPoolSize{
            vk::DescriptorType::eUniformBuffer,
            static_cast<uint32_t>(maxFramesInFlight)},
        vk::DescriptorPoolSize{
            vk::DescriptorType::eCombinedImageSampler,
            static_cast<uint32_t>(maxFramesInFlight)}};

    auto descriptorPool = vk::raii::DescriptorPool{
        r.ctx.device,
        vk::DescriptorPoolCreateInfo{
            vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
            static_cast<uint32_t>(maxFramesInFlight),
            poolSizes}};

    auto descriptorSetAllocateInfo =
        vk::DescriptorSetAllocateInfo{descriptorPool, descriptorSetLayouts};

    auto descriptorSets =
        r.ctx.device.allocateDescriptorSets(descriptorSetAllocateInfo);

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        auto descriptorBufferInfo =
            vk::DescriptorBufferInfo{sceneBuffers[i].buffer, 0, sizeof(Scene)};
        auto descriptorImageInfo = vk::DescriptorImageInfo{
            sampler,
            textureImageView,
            vk::ImageLayout::eShaderReadOnlyOptimal};
        auto descriptorWrites = std::array{
            vk::WriteDescriptorSet{
                descriptorSets[i],
                0,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                {},
                &descriptorBufferInfo},
            vk::WriteDescriptorSet{
                descriptorSets[i],
                1,
                0,
                1,
                vk::DescriptorType::eCombinedImageSampler,
                &descriptorImageInfo}};
        r.ctx.device.updateDescriptorSets(descriptorWrites, {});
    }

    auto pipelineLayout =
        vk::raii::PipelineLayout{r.ctx.device, {{}, descriptorSetLayouts}};

    auto graphicsPipeline = createPipeline(
        r.ctx.device,
        r.ctx.imageFormat,
        r.ctx.depthFormat,
        shaderModule,
        pipelineLayout);

    uint32_t frameRingCurrent = 0;
    uint32_t currentFrame     = 0;
    uint32_t totalFrames      = 0;

    bool swapchainNeedRebuild = false;

    bool      running = true;
    SDL_Event e;
    while (running) {
        if (totalFrames % 10 == 0) {
            fmt::print(stderr, "\n{} ", totalFrames);
        }
        fmt::print(stderr, ".");
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        // begin frame

        // check swapchain rebuild
        if (swapchainNeedRebuild) {
        }

        // get current frame data
        auto &cmdPool   = commandPools[frameRingCurrent];
        auto &cmdBuffer = commandBuffers[frameRingCurrent];
        auto  waitValue = frameNumbers[frameRingCurrent];

        {
            // wait semaphore frame (n - numFrames)
            auto res = r.ctx.device.waitSemaphores(
                vk::SemaphoreWaitInfo{{}, *frameTimelineSemaphore, waitValue},
                std::numeric_limits<uint64_t>::max());
        }

        // reset current frame's command pool to reuse the command buffer
        cmdPool.reset();
        cmdBuffer.begin(
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
            // continue;
        }

        // update uniform buffers
        auto scene = Scene{
            .model      = modelMatrix,
            .view       = viewMatrix,
            .projection = projectionMatrix};

        vmaCopyMemoryToAllocation(
            allocator.allocator,
            &scene,
            sceneBuffers[currentFrame].allocation,
            0,
            sizeof(Scene));

        // color attachment image to render to: vk::RenderingAttachmentInfo
        auto renderingColorAttachmentInfo = vk::RenderingAttachmentInfo{
            swapchainImageViews[nextImageIndex],
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearColorValue{std::array{0.0f, 0.f, 0.0f, 1.0f}}};

        // depth attachment buffer: vk::RenderingAttachmentInfo
        auto renderingDepthAttachmentInfo = vk::RenderingAttachmentInfo{
            depthImageView,
            vk::ImageLayout::eAttachmentOptimal,
            {},
            {},
            {},
            vk::AttachmentLoadOp::eClear,
            vk::AttachmentStoreOp::eStore,
            vk::ClearDepthStencilValue{1.0f, 0}};

        // vk::RenderingInfo
        auto renderingInfo = vk::RenderingInfo{
            {},
            vk::Rect2D{{}, r.ctx.windowExtent},
            1,
            {},
            renderingColorAttachmentInfo,
            &renderingDepthAttachmentInfo};

        // transition swapchain image layout:

        cmdTransitionImageLayout(
            cmdBuffer,
            swapchainImages[nextImageIndex],
            vk::ImageLayout::eGeneral,
            vk::ImageLayout::eColorAttachmentOptimal);

        cmdBuffer.beginRendering(renderingInfo);

        cmdBuffer.setViewportWithCount(
            vk::Viewport(
                0.0f,
                0.0f,
                r.ctx.windowExtent.width,
                r.ctx.windowExtent.height,
                0.0f,
                1.0f));
        cmdBuffer.setScissorWithCount(
            vk::Rect2D{vk::Offset2D(0, 0), r.ctx.windowExtent});

        cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline);

        // bind texture resources passed to shader
        cmdBuffer.bindDescriptorSets2(
            vk::BindDescriptorSetsInfo{
                vk::ShaderStageFlagBits::eAllGraphics,
                pipelineLayout,
                0,
                *descriptorSets[currentFrame]});

        // bind vertex data
        cmdBuffer.bindVertexBuffers(0, vertexBuffer.buffer, {0});

        // bind index data
        cmdBuffer.bindIndexBuffer(indexBuffer.buffer, 0, vk::IndexType::eUint16);

        cmdBuffer.drawIndexed(indexData.size(), 1, 0, 0, 0);

        cmdBuffer.endRendering();

        // ???transition image layout eColorAttachmentOptimal -> ePresentSrcKHR
        cmdTransitionImageLayout(
            cmdBuffer,
            swapchainImages[nextImageIndex],
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR);
        cmdBuffer.end();

        // end frame

        // prepare to submit the current frame for rendering
        // add the swapchain semaphore to wait for the image to be available

        uint64_t signalFrameValue      = waitValue + maxFramesInFlight;
        frameNumbers[frameRingCurrent] = signalFrameValue;

        auto waitSemaphore = vk::SemaphoreSubmitInfo{
            imageAvailable[currentFrame],
            {},
            vk::PipelineStageFlagBits2::eColorAttachmentOutput};

        auto signalSemaphores = std::array{
            vk::SemaphoreSubmitInfo{
                renderFinished[nextImageIndex],
                {},
                vk::PipelineStageFlagBits2::eColorAttachmentOutput},
            vk::SemaphoreSubmitInfo{
                frameTimelineSemaphore,
                signalFrameValue,
                vk::PipelineStageFlagBits2::eColorAttachmentOutput}};

        auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{cmdBuffer};

        auto submitInfo = vk::SubmitInfo2{
            {},
            waitSemaphore,
            commandBufferSubmitInfo,
            signalSemaphores};

        r.ctx.graphicsQueue.submit2(
            vk::SubmitInfo2{
                {},
                waitSemaphore,
                commandBufferSubmitInfo,
                signalSemaphores});

        // present frame
        auto presentResult = r.ctx.presentQueue.presentKHR(
            vk::PresentInfoKHR{
                *renderFinished[nextImageIndex],
                *r.ctx.swapchain,
                nextImageIndex});

        if (presentResult == vk::Result::eErrorOutOfDateKHR) {
            swapchainNeedRebuild = true;
        }

        else if (!(result == vk::Result::eSuccess
                   || result == vk::Result::eSuboptimalKHR)) {
            throw std::exception{};
        }

        // advance to the next frame in the swapchain
        currentFrame = (currentFrame + 1) % maxFramesInFlight;

        // move to the next frame
        frameRingCurrent = (frameRingCurrent + 1) % maxFramesInFlight;

        ++totalFrames;
    }

    return 0;
}
