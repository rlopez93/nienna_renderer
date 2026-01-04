#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "UniqueImage.hpp"
#include "vma.hpp"

struct Device;

struct VmaPoolDeleter {

    VmaAllocator allocator;
    auto         operator()(VmaPool_T *pool) const noexcept -> void
    {
        if (!allocator || !pool) {
            return;
        }

        vmaDestroyPool(allocator, pool);
    }
};

using UniqueVmaPool = std::unique_ptr<VmaPool_T, VmaPoolDeleter>;

class RenderTargetAllocator
{
  public:
    explicit RenderTargetAllocator(VmaAllocator vma);

    [[nodiscard]]
    auto handle() const -> VmaAllocator;

    enum class PoolKind : std::uint8_t {
        kColor,
        kDepth,
    };

    [[nodiscard]]
    auto createImage(
        const vk::ImageCreateInfo &info,
        PoolKind                   kind) -> UniqueImage;

  private:
    VmaAllocator  vmaAllocator;
    UniqueVmaPool colorPool;
    UniqueVmaPool depthPool;

    [[nodiscard]]
    auto ensurePool(
        PoolKind                   kind,
        const vk::ImageCreateInfo &info) -> VmaPool;
};

struct ColorTarget {
    vk::Format          format = vk::Format::eUndefined;
    UniqueImage         image{};
    vk::raii::ImageView view = nullptr;

    auto recreate(
        Device                &device,
        RenderTargetAllocator &alloc,
        vk::Extent2D           extent,
        vk::Format             swapchainFormat) -> void;

    [[nodiscard]]
    auto range() const -> vk::ImageSubresourceRange;
};

struct DepthTarget {
    vk::Format          format = vk::Format::eUndefined;
    UniqueImage         image{};
    vk::raii::ImageView view = nullptr;

    auto recreate(
        Device                &device,
        RenderTargetAllocator &alloc,
        vk::Extent2D           extent,
        vk::Format             depthFormat) -> void;

    [[nodiscard]]
    auto range() const -> vk::ImageSubresourceRange;
};

struct RenderTargets {
    RenderTargetAllocator allocator;
    ColorTarget           sceneColorLdr{};
    DepthTarget           mainDepth{};
    vk::Extent2D          extent{};

    RenderTargets(
        Device      &device,
        VmaAllocator vma,
        vk::Extent2D extent_,
        vk::Format   swapchainFormat,
        vk::Format   depthFormat);

    auto recreate(
        Device      &device,
        vk::Extent2D extent_,
        vk::Format   swapchainFormat,
        vk::Format   depthFormat) -> void;

    [[nodiscard]]
    auto images() const -> std::vector<vk::Image>;
};
