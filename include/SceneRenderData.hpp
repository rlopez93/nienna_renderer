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

struct SceneRenderData {
    void create(
        const Scene &scene,
        Device      &device,
        Command     &command,
        Allocator   &allocator);

    void updateDescriptorSet(
        Device            &device,
        vk::DescriptorSet  descriptorSet,
        const Buffer      &transformUBO,
        const Buffer      &lightUBO,
        vk::raii::Sampler &sampler) const;

    // draw list consumed by Renderer
    std::vector<DrawItem> draws;

    // static GPU data consumed by Renderer
    std::vector<Buffer> indexBuffers;
    std::vector<Buffer> vertexBuffers;

    std::vector<Image>               textureImages;
    std::vector<vk::raii::ImageView> textureImageViews;
};
