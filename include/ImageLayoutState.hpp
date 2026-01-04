#pragma once

#include <cstdint>
#include <unordered_map>

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

enum class ImageUse : std::uint8_t {
    kColorAttachmentWrite,
    kDepthAttachmentWrite,
    kShaderSampledRead,
    kTransferDstWrite,
    kPresent,
};

struct ImageUseInfo {
    vk::ImageLayout         layout{};
    vk::PipelineStageFlags2 stage{};
    vk::AccessFlags2        access{};
};

[[nodiscard]]
auto imageUseInfo(ImageUse use) -> ImageUseInfo;

struct ImageLayoutState {
    void reset();

    void resetForSwapchain(const std::vector<vk::Image> &images);

    void transition(
        vk::raii::CommandBuffer  &cmd,
        vk::Image                 image,
        vk::ImageSubresourceRange range,
        ImageUse                  newUse);

  private:
    struct Entry {
        bool         valid = false;
        ImageUse     use{};
        ImageUseInfo info{};
    };

    std::unordered_map<VkImage, Entry> entries;
};
