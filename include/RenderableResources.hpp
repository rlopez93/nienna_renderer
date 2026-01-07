#pragma once

#include "Allocator.hpp"
#include "Buffer.hpp"
#include "Command.hpp"
#include "Device.hpp"
#include "DrawItem.hpp"
#include "Image.hpp"
#include "RenderAsset.hpp"

#include <unordered_map>
#include <vector>

#include <vulkan/vulkan_hash.hpp>
#include <vulkan/vulkan_raii.hpp>

struct SceneView;

struct RenderableResources {
    void create(
        const RenderAsset &asset,
        const SceneView   &sceneView,
        Device            &device,
        Command           &command,
        Allocator         &allocator);

    void updateDescriptorSet(
        Device           &device,
        vk::DescriptorSet descriptorSet,
        const Buffer     &frameUBO,
        const Buffer     &nodeInstancesSSBO) const;

    // draw list consumed by Renderer
    std::vector<DrawItem> draws;

    // static GPU data consumed by Renderer
    std::vector<Buffer> indexBuffers;
    std::vector<Buffer> vertexBuffers;

    std::vector<Image>               textureImages;
    std::vector<vk::raii::ImageView> srgbTextureImageViews;
    std::vector<vk::raii::ImageView> linearTextureImageViews;

    Buffer materialsSSBO;

    std::vector<vk::Sampler>       samplerHandles;
    std::vector<vk::raii::Sampler> uniqueSamplers;

    std::unordered_map<vk::SamplerCreateInfo, std::uint32_t> samplerCache;
};
