#include "SceneResources.hpp"
#include "Scene.hpp"

#include <ranges>

auto SceneRenderData::create(
    const Scene &scene,
    Device      &device,
    Command     &command,
    Allocator   &allocator,
    uint64_t     maxFramesInFlight) -> void
{
    command.beginSingleTime();

    for (const auto &mesh : scene.meshes) {
        buffers.vertex.emplace_back(allocator.createBufferAndUploadData(
            command.buffer,
            mesh.primitives,
            vk::BufferUsageFlagBits2::eVertexBuffer));

        buffers.index.emplace_back(allocator.createBufferAndUploadData(
            command.buffer,
            mesh.indices,
            vk::BufferUsageFlagBits2::eIndexBuffer));
    }

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        buffers.uniform.emplace_back(allocator.createBuffer(
            sizeof(Transform) * scene.meshes.size(),
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            false,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    }

    for (auto i : std::views::iota(0u, maxFramesInFlight)) {
        buffers.light.emplace_back(allocator.createBuffer(
            sizeof(Light),
            vk::BufferUsageFlagBits2::eUniformBuffer
                | vk::BufferUsageFlagBits2::eTransferDst,
            false,
            VMA_MEMORY_USAGE_AUTO,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
                | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    }

    for (const auto &texture : scene.textures) {
        textureBuffers.image.emplace_back(allocator.createImageAndUploadData(
            command.buffer,
            texture.data,
            vk::ImageCreateInfo{
                {},
                vk::ImageType::e2D,
                vk::Format::eR8G8B8A8Srgb,
                vk::Extent3D(texture.extent, 1),
                1,
                1,
                vk::SampleCountFlagBits::e1,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eSampled},
            vk::ImageLayout::eShaderReadOnlyOptimal));

        textureBuffers.imageView.emplace_back(device.handle.createImageView(
            vk::ImageViewCreateInfo{
                {},
                textureBuffers.image.back().image,
                vk::ImageViewType::e2D,
                vk::Format::eR8G8B8A8Srgb,
                {},
                vk::ImageSubresourceRange{
                    vk::ImageAspectFlagBits::eColor,
                    0,
                    1,
                    0,
                    1}}));
    }

    command.endSingleTime(device);

    // Clear any previous draw list
    draws.clear();
    draws.reserve(scene.meshes.size());

    for (const auto &[meshIndex, mesh] : std::ranges::views::enumerate(scene.meshes)) {

        DrawItem draw{
            draw.indexCount   = static_cast<uint32_t>(mesh.indices.size()),
            draw.firstIndex   = 0,
            draw.vertexOffset = 0,
            // Shader-facing indices
            draw.transformIndex = meshIndex,
            draw.textureIndex   = mesh.textureIndex.has_value()
                                    ? static_cast<int32_t>(*mesh.textureIndex)
                                    : -1};

        draw.baseColor = mesh.color;

        draws.emplace_back(draw);
    }
}

void SceneRenderData::updateDescriptorSet(
    Device            &device,
    vk::DescriptorSet  descriptorSet,
    uint32_t           frameIndex,
    uint32_t           meshCount,
    vk::raii::Sampler &sampler) const
{
    auto writes      = std::vector<vk::WriteDescriptorSet>{};
    auto bufferInfos = std::vector<vk::DescriptorBufferInfo>{};

    constexpr auto TransformStride = vk::DeviceSize{sizeof(Transform)};

    for (auto meshIndex : std::views::iota(0u, meshCount)) {
        bufferInfos.emplace_back(
            buffers.uniform[frameIndex].buffer,
            TransformStride * meshIndex,
            TransformStride);
    }

    if (!bufferInfos.empty()) {
        writes.emplace_back(
            vk::WriteDescriptorSet{
                descriptorSet,
                0,
                0,
                vk::DescriptorType::eUniformBuffer,
                {},
                bufferInfos});
    }

    auto lightInfo =
        vk::DescriptorBufferInfo{buffers.light[frameIndex].buffer, 0, sizeof(Light)};

    writes.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            2,
            0,
            1,
            vk::DescriptorType::eUniformBuffer,
            {},
            &lightInfo});

    auto images = std::vector<vk::DescriptorImageInfo>{};
    for (const auto &view : textureBuffers.imageView) {
        images.emplace_back(*sampler, *view, vk::ImageLayout::eShaderReadOnlyOptimal);
    }

    if (!images.empty()) {
        writes.emplace_back(
            vk::WriteDescriptorSet{
                descriptorSet,
                1,
                0,
                vk::DescriptorType::eCombinedImageSampler,
                images});
    }

    device.handle.updateDescriptorSets(writes, {});
}
