#include "SceneRenderData.hpp"

#include "Asset.hpp"
#include "MaterialPacking.hpp"
#include "SceneDrawList.hpp"

#include <cstdint>
#include <numeric>
#include <ranges>
#include <vector>

namespace
{

using SamplerCache = std::unordered_map<vk::SamplerCreateInfo, std::uint32_t>;

using UniqueSamplers = std::vector<vk::raii::Sampler>;

[[nodiscard]]
auto makeHashable(vk::SamplerCreateInfo samplerCreateInfo) -> vk::SamplerCreateInfo
{
    samplerCreateInfo.pNext = nullptr;

    if (!samplerCreateInfo.anisotropyEnable) {
        samplerCreateInfo.maxAnisotropy = 1.0f;
    }

    if (!samplerCreateInfo.compareEnable) {
        samplerCreateInfo.compareOp = vk::CompareOp::eNever;
    }

    if (samplerCreateInfo.unnormalizedCoordinates) {
        samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    }

    return samplerCreateInfo;
}

[[nodiscard]]
auto getOrCreateSamplerIndex(
    Device               &device,
    vk::SamplerCreateInfo samplerCreateInfo,
    SamplerCache         &samplerCache,
    UniqueSamplers       &uniqueSamplers) -> std::uint32_t
{
    samplerCreateInfo = makeHashable(samplerCreateInfo);

    const auto samplerCacheIterator = samplerCache.find(samplerCreateInfo);

    if (samplerCacheIterator != samplerCache.end()) {
        return samplerCacheIterator->second;
    }

    const auto uniqueSamplerIndex = static_cast<std::uint32_t>(uniqueSamplers.size());

    uniqueSamplers.emplace_back(device.handle, samplerCreateInfo);

    samplerCache.emplace(samplerCreateInfo, uniqueSamplerIndex);

    return uniqueSamplerIndex;
}

} // namespace
auto SceneRenderData::create(
    const Asset         &asset,
    const SceneDrawList &sceneDrawList,
    Device              &device,
    Command             &command,
    Allocator           &allocator) -> void
{
    draws = sceneDrawList.draws;

    auto meshOffsets = std::vector<std::uint32_t>(asset.meshes.size() + 1, 0u);
    // each mesh's offset in our geometry buffers (vertex/index buffers) can be found by
    // generating prefix sums of the primitive counts in each mesh, using
    // std::transform_inclusive_scan()
    // meshOffsets[m] is the geometry index for the first primitive in mesh m
    (void)std::transform_inclusive_scan(
        asset.meshes.begin(),
        asset.meshes.end(),
        meshOffsets.begin() + 1, // we start our inclusive_scan at meshOffsets[1] so
                                 // that meshOffsets.front() == 0
                                 // and meshOffsets.back() == total primitive count
        std::plus<>{},
        [](const auto &mesh) {
            return static_cast<std::uint32_t>(mesh.primitives.size());
        });

    // we use the mesh offsets and the primitive index to get the final geometry index
    // for every draw call
    for (DrawItem &draw : draws) {
        draw.geometryIndex = meshOffsets[draw.meshIndex] + draw.primitiveIndex;
    }

    vertexBuffers.clear();
    indexBuffers.clear();

    // meshOffsets.back() is the total primitive count
    const auto primitiveCount = meshOffsets.back();

    vertexBuffers.reserve(primitiveCount);
    indexBuffers.reserve(primitiveCount);

    textureImages.clear();
    srgbTextureImageViews.clear();
    linearTextureImageViews.clear();

    textureImages.reserve(asset.textures.size());
    srgbTextureImageViews.reserve(asset.textures.size());
    linearTextureImageViews.reserve(asset.textures.size());

    samplerHandles.clear();
    uniqueSamplers.clear();
    samplerCache.clear();

    samplerHandles.resize(asset.textures.size());

    command.beginSingleTime();

    for (const auto &mesh : asset.meshes) {
        for (const auto &primitive : mesh.primitives) {

            vertexBuffers.push_back(allocator.createBufferAndUploadData(
                command.buffer,
                primitive.vertices,
                vk::BufferUsageFlagBits2::eVertexBuffer));

            indexBuffers.push_back(allocator.createBufferAndUploadData(
                command.buffer,
                primitive.indices,
                vk::BufferUsageFlagBits2::eIndexBuffer));
        }
    }

    std::vector<MaterialData> packedMaterials{};
    packedMaterials.reserve(asset.materials.size());

    if (asset.materials.empty()) {
        packedMaterials.push_back(packMaterialData(Material{}));
    } else {
        for (const Material &material : asset.materials) {
            packedMaterials.push_back(packMaterialData(material));
        }
    }

    materialsSSBO = allocator.createBufferAndUploadData(
        command.buffer,
        packedMaterials,
        vk::BufferUsageFlagBits2::eStorageBuffer);

    for (const Texture &texture : asset.textures) {

        const vk::Extent3D extent3D{texture.extent.width, texture.extent.height, 1u};

        vk::ImageCreateInfo imageInfo{};
        imageInfo.flags = vk::ImageCreateFlagBits::eMutableFormat;

        imageInfo.imageType = vk::ImageType::e2D;
        imageInfo.format    = vk::Format::eR8G8B8A8Unorm;

        imageInfo.extent      = extent3D;
        imageInfo.mipLevels   = 1u;
        imageInfo.arrayLayers = 1u;

        imageInfo.samples = vk::SampleCountFlagBits::e1;
        imageInfo.tiling  = vk::ImageTiling::eOptimal;

        imageInfo.usage = vk::ImageUsageFlagBits::eSampled;

        Image image = allocator.createImageAndUploadData(
            command.buffer,
            texture.rgba8,
            imageInfo,
            vk::ImageLayout::eShaderReadOnlyOptimal);

        textureImages.push_back(image);

        vk::ImageSubresourceRange range{};
        range.aspectMask = vk::ImageAspectFlagBits::eColor;

        range.baseMipLevel   = 0u;
        range.levelCount     = 1u;
        range.baseArrayLayer = 0u;
        range.layerCount     = 1u;

        srgbTextureImageViews.push_back(device.handle.createImageView(
            vk::ImageViewCreateInfo{
                {},
                textureImages.back().image,
                vk::ImageViewType::e2D,
                vk::Format::eR8G8B8A8Srgb,
                {},
                range}));

        linearTextureImageViews.push_back(device.handle.createImageView(
            vk::ImageViewCreateInfo{
                {},
                textureImages.back().image,
                vk::ImageViewType::e2D,
                vk::Format::eR8G8B8A8Unorm,
                {},
                range}));
    }

    command.endSingleTime(device);

    for (const auto &[textureIndex, texture] : std::views::enumerate(asset.textures)) {

        const std::uint32_t uniqueSamplerIndex = getOrCreateSamplerIndex(
            device,
            texture.samplerInfo,
            samplerCache,
            uniqueSamplers);

        samplerHandles[textureIndex] = *uniqueSamplers[uniqueSamplerIndex];
    }
}

void SceneRenderData::updateDescriptorSet(
    Device           &device,
    vk::DescriptorSet descriptorSet,
    const Buffer     &frameUBO,
    const Buffer     &objectsSSBO) const
{
    auto descriptorWrites = std::vector<vk::WriteDescriptorSet>{};

    const auto frameBufferInfo =
        vk::DescriptorBufferInfo{frameUBO.buffer, 0, sizeof(FrameUniforms)};

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingFrameUniforms,
            0,
            vk::DescriptorType::eUniformBuffer,
            {},
            frameBufferInfo});
    const auto objectsBufferInfo = vk::DescriptorBufferInfo{
        objectsSSBO.buffer,
        0,
        vk::WholeSize,
    };

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingObjectData,
            0,
            vk::DescriptorType::eStorageBuffer,
            {},
            objectsBufferInfo,
        });

    const auto materialsBufferInfo = vk::DescriptorBufferInfo{
        materialsSSBO.buffer,
        0,
        vk::WholeSize,
    };

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingMaterialData,
            0,
            vk::DescriptorType::eStorageBuffer,
            {},
            materialsBufferInfo,
        });

    std::vector<vk::DescriptorImageInfo> srgbTextureImageViewInfos;
    srgbTextureImageViewInfos.reserve(srgbTextureImageViews.size());

    for (const auto &srgbView : srgbTextureImageViews) {
        srgbTextureImageViewInfos.emplace_back(
            vk::DescriptorImageInfo{
                vk::Sampler{},
                *srgbView,
                vk::ImageLayout::eShaderReadOnlyOptimal,
            });
    }

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingImagesSrgb,
            0,
            vk::DescriptorType::eSampledImage,
            srgbTextureImageViewInfos,
        });

    std::vector<vk::DescriptorImageInfo> linearTextureImageViewInfos;
    linearTextureImageViewInfos.reserve(linearTextureImageViews.size());

    for (const auto &linearView : linearTextureImageViews) {
        linearTextureImageViewInfos.emplace_back(
            vk::DescriptorImageInfo{
                vk::Sampler{},
                *linearView,
                vk::ImageLayout::eShaderReadOnlyOptimal,
            });
    }

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingImagesLinear,
            0,
            vk::DescriptorType::eSampledImage,
            linearTextureImageViewInfos,
        });

    std::vector<vk::DescriptorImageInfo> samplerDescriptorInfos;
    samplerDescriptorInfos.reserve(samplerHandles.size());

    for (vk::Sampler sampler : samplerHandles) {
        samplerDescriptorInfos.emplace_back(
            vk::DescriptorImageInfo{
                sampler,
                vk::ImageView{},
                vk::ImageLayout{},
            });
    }

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingSamplers,
            0,
            vk::DescriptorType::eSampler,
            samplerDescriptorInfos,
        });

    device.handle.updateDescriptorSets(descriptorWrites, {});
}
