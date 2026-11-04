#include "RenderableResources.hpp"

#include "MaterialPacking.hpp"
#include "RenderAsset.hpp"
#include "SceneView.hpp"
#include "ShaderInterfaceTypes.hpp"

#include <cstdint>
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
    Device&               device,
    vk::SamplerCreateInfo samplerCreateInfo,
    SamplerCache&         samplerCache,
    UniqueSamplers&       uniqueSamplers) -> std::uint32_t
{
    samplerCreateInfo = makeHashable(samplerCreateInfo);

    auto const samplerCacheIterator = samplerCache.find(samplerCreateInfo);

    if (samplerCacheIterator != samplerCache.end()) {
        return samplerCacheIterator->second;
    }

    auto const uniqueSamplerIndex = static_cast<std::uint32_t>(uniqueSamplers.size());

    uniqueSamplers.emplace_back(device.handle, samplerCreateInfo);

    samplerCache.emplace(samplerCreateInfo, uniqueSamplerIndex);

    return uniqueSamplerIndex;
}

} // namespace
auto RenderableResources::create(
    RenderAsset const& asset,
    SceneView const&   sceneView,
    Device&            device,
    Command&           command,
    Allocator&         allocator) -> void
{
    draws = sceneView.draws;

    vertexBuffers.clear();
    indexBuffers.clear();

    // meshOffsets.back() is the total primitive count
    // const auto primitiveCount = meshOffsets.back();
    //
    // vertexBuffers.reserve(primitiveCount);
    // indexBuffers.reserve(primitiveCount);

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

    // TODO: create large geometry buffers to "flatten out" vertex/indexBuffers
    for (auto const& mesh : asset.meshes) {
        for (auto const& primitive : mesh.submeshes) {

            vertexBuffers.push_back(allocator.createBufferAndUploadData(
                command,
                primitive.vertices,
                vk::BufferUsageFlagBits2::eVertexBuffer));

            indexBuffers.push_back(allocator.createBufferAndUploadData(
                command,
                primitive.indices,
                vk::BufferUsageFlagBits2::eIndexBuffer));
        }
    }

    std::vector<MaterialData> packedMaterials{};
    packedMaterials.reserve(asset.materials.size());

    if (asset.materials.empty()) {
        packedMaterials.push_back(packMaterialData(Material{}));
    } else {
        for (Material const& material : asset.materials) {
            packedMaterials.push_back(packMaterialData(material));
        }
    }

    materialsSSBO = allocator.createBufferAndUploadData(
        command,
        packedMaterials,
        vk::BufferUsageFlagBits2::eStorageBuffer);

    for (Texture const& texture : asset.textures) {

        vk::Extent3D const extent3D{texture.extent.width, texture.extent.height, 1u};

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
            command,
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

    for (auto const& [textureIndex, texture] : std::views::enumerate(asset.textures)) {

        std::uint32_t const uniqueSamplerIndex = getOrCreateSamplerIndex(
            device,
            texture.samplerInfo,
            samplerCache,
            uniqueSamplers);

        samplerHandles[textureIndex] = *uniqueSamplers[uniqueSamplerIndex];
    }
}

void RenderableResources::updateDescriptorSet(
    Device&           device,
    vk::DescriptorSet descriptorSet,
    Buffer const&     frameUBO,
    Buffer const&     nodeInstancesSSBO) const
{
    auto descriptorWrites = std::vector<vk::WriteDescriptorSet>{};

    auto const frameBufferInfo =
        vk::DescriptorBufferInfo{frameUBO.buffer, 0, sizeof(FrameUniforms)};

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingFrameUniforms,
            0,
            vk::DescriptorType::eUniformBuffer,
            {},
            frameBufferInfo});
    auto const nodeInstancesBufferInfo = vk::DescriptorBufferInfo{
        nodeInstancesSSBO.buffer,
        0,
        vk::WholeSize,
    };

    descriptorWrites.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            kBindingNodeInstanceData,
            0,
            vk::DescriptorType::eStorageBuffer,
            {},
            nodeInstancesBufferInfo,
        });

    auto const materialsBufferInfo = vk::DescriptorBufferInfo{
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

    for (auto const& srgbView : srgbTextureImageViews) {
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

    for (auto const& linearView : linearTextureImageViews) {
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
