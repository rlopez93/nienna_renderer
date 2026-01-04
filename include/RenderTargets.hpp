#pragma once

#include <cstdint>
#include <vector>

#include <vulkan/vulkan_raii.hpp>

#include "Image.hpp"
#include "vma.hpp"

struct Device;

class RenderTargetAllocator
{
  public:
    RenderTargetAllocator() = default;

    explicit RenderTargetAllocator(VmaAllocator vma);

    ~RenderTargetAllocator();

    [[nodiscard]]
    auto vma() const -> VmaAllocator;

    auto setAllocator(VmaAllocator vma) -> void;

    enum class PoolKind : std::uint8_t {
        kColor,
        kDepth,
    };

    [[nodiscard]]
    auto createImage(
        const vk::ImageCreateInfo &info,
        PoolKind                   kind) -> Image;

    auto destroyImage(Image img) const -> void;

  private:
    VmaAllocator allocator = nullptr;
    VmaPool      colorPool = nullptr;
    VmaPool      depthPool = nullptr;

    [[nodiscard]]
    auto ensurePool(
        PoolKind                   kind,
        const vk::ImageCreateInfo &info) -> VmaPool;

    auto destroyPool(VmaPool &pool) -> void;
};
class OwnedImage
{
  public:
    OwnedImage() = default;

    OwnedImage(
        VmaAllocator allocator_,
        Image        image_);

    ~OwnedImage();

    OwnedImage(const OwnedImage &)                     = delete;
    auto operator=(const OwnedImage &) -> OwnedImage & = delete;

    OwnedImage(OwnedImage &&other) noexcept;
    auto operator=(OwnedImage &&other) noexcept -> OwnedImage &;

    [[nodiscard]]
    auto vkImage() const -> vk::Image;

    auto reset() -> void;

  private:
    VmaAllocator allocator = nullptr;
    Image        image{};
};

struct ColorTarget {
    vk::Format          format = vk::Format::eUndefined;
    OwnedImage          image{};
    vk::raii::ImageView view = nullptr;

    auto recreate(
        Device                &device,
        RenderTargetAllocator &alloc,
        vk::Extent2D           extent,
        vk::Format             swapchainFormat) -> void;

    [[nodiscard]]
    auto vkImage() const -> vk::Image;

    [[nodiscard]]
    auto range() const -> vk::ImageSubresourceRange;
};

struct DepthTarget {
    vk::Format          format = vk::Format::eUndefined;
    OwnedImage          image{};
    vk::raii::ImageView view = nullptr;

    auto recreate(
        Device                &device,
        RenderTargetAllocator &alloc,
        vk::Extent2D           extent,
        vk::Format             depthFormat) -> void;

    [[nodiscard]]
    auto vkImage() const -> vk::Image;

    [[nodiscard]]
    auto range() const -> vk::ImageSubresourceRange;
};

struct RenderTargets {
    RenderTargetAllocator allocator{};
    ColorTarget           sceneColorLdr{};
    DepthTarget           mainDepth{};
    vk::Extent2D          extent{};

    RenderTargets() = default;

    RenderTargets(
        Device      &device,
        VmaAllocator vma,
        vk::Extent2D extent_,
        vk::Format   swapchainFormat,
        vk::Format   depthFormat);

    auto recreate(
        Device      &device,
        VmaAllocator vma,
        vk::Extent2D extent_,
        vk::Format   swapchainFormat,
        vk::Format   depthFormat) -> void;

    [[nodiscard]]
    auto images() const -> std::vector<vk::Image>;
};
