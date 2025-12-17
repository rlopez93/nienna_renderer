// ResourceUpdate.cpp

#include "ResourceUpdate.hpp"
#include "Buffer.hpp"
#include "FrameContext.hpp"

#include <ranges>

auto ResourceUpdate::updatePerFrame(
    vk::raii::Device                       &device,
    FrameContext                           &frames,
    const std::vector<Buffer>              &transformUBOs,
    const std::vector<Buffer>              &lightUBOs,
    uint32_t                                meshCount,
    const std::vector<vk::raii::ImageView> &textureImageViews,
    vk::raii::Sampler                      &sampler) -> void
{
    for (auto frameIndex : std::views::iota(0u, frames.maxFramesInFlight)) {

        auto writes  = std::vector<vk::WriteDescriptorSet>{};
        auto buffers = std::vector<vk::DescriptorBufferInfo>{};

        for (auto meshIndex : std::views::iota(0u, meshCount)) {
            buffers.emplace_back(
                transformUBOs[frameIndex].buffer,
                sizeof(Transform) * meshIndex,
                sizeof(Transform));
        }

        if (!buffers.empty()) {
            writes.emplace_back(
                vk::WriteDescriptorSet{
                    *frames.resourceBindings[frameIndex],
                    0,
                    0,
                    vk::DescriptorType::eUniformBuffer,
                    {},
                    buffers});
        }

        auto lightInfo =
            vk::DescriptorBufferInfo{lightUBOs[frameIndex].buffer, 0, sizeof(Light)};

        writes.emplace_back(
            vk::WriteDescriptorSet{
                *frames.resourceBindings[frameIndex],
                2,
                0,
                1,
                vk::DescriptorType::eUniformBuffer,
                {},
                &lightInfo});

        auto images = std::vector<vk::DescriptorImageInfo>{};
        for (const auto &view : textureImageViews) {
            images.emplace_back(
                *sampler,
                *view,
                vk::ImageLayout::eShaderReadOnlyOptimal);
        }

        if (!images.empty()) {
            writes.emplace_back(
                vk::WriteDescriptorSet{
                    *frames.resourceBindings[frameIndex],
                    1,
                    0,
                    vk::DescriptorType::eCombinedImageSampler,
                    images});
        }

        device.updateDescriptorSets(writes, {});
    }
}
