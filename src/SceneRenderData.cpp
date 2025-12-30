#include "SceneRenderData.hpp"
#include "Scene.hpp"

#include <ranges>

auto SceneRenderData::create(
    const Scene &scene,
    Device      &device,
    Command     &command,
    Allocator   &allocator) -> void
{
    command.beginSingleTime();

    for (const auto &mesh : scene.meshes) {
        vertexBuffers.emplace_back(allocator.createBufferAndUploadData(
            command.buffer,
            mesh.primitives,
            vk::BufferUsageFlagBits2::eVertexBuffer));

        indexBuffers.emplace_back(allocator.createBufferAndUploadData(
            command.buffer,
            mesh.indices,
            vk::BufferUsageFlagBits2::eIndexBuffer));
    }

    // for (auto i : std::views::iota(0u, maxFramesInFlight)) {
    //     buffers.uniform.emplace_back(allocator.createBuffer(
    //         sizeof(Transform) * scene.meshes.size(),
    //         vk::BufferUsageFlagBits2::eUniformBuffer
    //             | vk::BufferUsageFlagBits2::eTransferDst,
    //         false,
    //         VMA_MEMORY_USAGE_AUTO,
    //         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    //             | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    // }
    //
    // for (auto i : std::views::iota(0u, maxFramesInFlight)) {
    //     buffers.light.emplace_back(allocator.createBuffer(
    //         sizeof(Light),
    //         vk::BufferUsageFlagBits2::eUniformBuffer
    //             | vk::BufferUsageFlagBits2::eTransferDst,
    //         false,
    //         VMA_MEMORY_USAGE_AUTO,
    //         VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    //             | VMA_ALLOCATION_CREATE_MAPPED_BIT));
    // }
    //
    for (const auto &texture : scene.textures) {
        textureImages.emplace_back(allocator.createImageAndUploadData(
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

        textureImageViews.emplace_back(device.handle.createImageView(
            vk::ImageViewCreateInfo{
                {},
                textureImages.back().image,
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
            draw.textureIndex   = mesh.textureIndex};

        draw.baseColor = mesh.color;

        draws.emplace_back(draw);
    }
}
void SceneRenderData::updateDescriptorSet(
    Device            &device,
    vk::DescriptorSet  descriptorSet,
    const Buffer      &transformUBO,
    const Buffer      &lightUBO,
    vk::raii::Sampler &sampler) const
{
    auto writes = std::vector<vk::WriteDescriptorSet>{};

    // binding 0: array of Transform structs (one per draw/mesh)
    auto transformInfos = std::vector<vk::DescriptorBufferInfo>{};
    transformInfos.reserve(draws.size());
    constexpr auto TransformStride = vk::DeviceSize{sizeof(Transform)};

    for (auto meshIndex : std::views::iota(0u, draws.size())) {
        transformInfos.emplace_back(
            transformUBO.buffer,
            TransformStride * meshIndex,
            TransformStride);
    }

    if (!transformInfos.empty()) {
        writes.emplace_back(
            vk::WriteDescriptorSet{
                descriptorSet,
                0, // binding
                0, // array element
                vk::DescriptorType::eUniformBuffer,
                {},
                transformInfos});
    }

    // binding 1: texture array
    auto images = std::vector<vk::DescriptorImageInfo>{};
    for (const auto &view : textureImageViews) {
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

    // binding 2: Light UBO
    auto lightInfo = vk::DescriptorBufferInfo{lightUBO.buffer, 0, sizeof(Light)};
    writes.emplace_back(
        vk::WriteDescriptorSet{
            descriptorSet,
            2,
            0,
            vk::DescriptorType::eUniformBuffer,
            {},
            lightInfo});

    device.handle.updateDescriptorSets(writes, {});
}
