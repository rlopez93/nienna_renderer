#include "SceneResources.hpp"
#include "Scene.hpp"

#include <ranges>

auto SceneResources::create(
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
}
