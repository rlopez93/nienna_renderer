// ResourceUpdate.hpp

#pragma once
#include <cstdint>
#include <vector>
#include <vulkan/vulkan_raii.hpp>

struct FrameContext;
struct Buffer;
struct Transform;
struct Light;

struct ResourceUpdate {
    static auto updatePerFrame(
        vk::raii::Device                       &device,
        FrameContext                           &frames,
        const std::vector<Buffer>              &transformUBOs,
        const std::vector<Buffer>              &lightUBOs,
        uint32_t                                meshCount,
        const std::vector<vk::raii::ImageView> &textureImageViews,
        vk::raii::Sampler                      &sampler) -> void;
};
