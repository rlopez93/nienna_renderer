#pragma once

#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Command.hpp"
#include "Device.hpp"
#include "DrawItem.hpp"
#include "Image.hpp"

#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct Scene;

struct SceneResources {
    struct Buffers {
        std::vector<Buffer> index;
        std::vector<Buffer> vertex;
        std::vector<Buffer> uniform;
        std::vector<Buffer> light;
    } buffers;

    struct TextureBuffers {
        std::vector<Image>               image;
        std::vector<vk::raii::ImageView> imageView;
    } textureBuffers;

    void create(
        const Scene &scene,
        Device      &device,
        Command     &command,
        Allocator   &allocator,
        uint64_t     maxFramesInFlight);

    void updateDescriptorSet(
        Device            &device,
        vk::DescriptorSet  descriptorSet,
        uint32_t           frameIndex,
        uint32_t           meshCount,
        vk::raii::Sampler &sampler) const;

    std::vector<DrawItem> draws;
};
