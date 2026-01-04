#include "RenderTargets.hpp"

#include <cassert>

#include "Device.hpp"
#include "Utility.hpp"

namespace
{

[[nodiscard]]
auto ldrFormatFromSwapchain(vk::Format fmt) -> vk::Format
{
    switch (fmt) {
    case vk::Format::eB8G8R8A8Srgb:
        return vk::Format::eB8G8R8A8Unorm;
    case vk::Format::eR8G8B8A8Srgb:
        return vk::Format::eR8G8B8A8Unorm;
    default:
        return fmt;
    }
}

[[nodiscard]]
auto depthAspectMask(vk::Format fmt) -> vk::ImageAspectFlags
{
    switch (fmt) {
    case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    default:
        return vk::ImageAspectFlagBits::eDepth;
    }
}

} // namespace

RenderTargetAllocator::RenderTargetAllocator(VmaAllocator vma)
    : vmaAllocator{vma},
      colorPool{
          nullptr,
          VmaPoolDeleter(vma)},
      depthPool{
          nullptr,
          VmaPoolDeleter(vma)}
{
}

[[nodiscard]]
auto RenderTargetAllocator::handle() const -> VmaAllocator
{
    return vmaAllocator;
}

auto RenderTargetAllocator::ensurePool(
    PoolKind                   kind,
    const vk::ImageCreateInfo &info) -> VmaPool
{
    assert(vmaAllocator != nullptr);

    auto &pool = (kind == PoolKind::kColor) ? colorPool : depthPool;

    if (pool) {
        return pool.get();
    }

    const VkImageCreateInfo vkInfo = *info;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    std::uint32_t memType = 0u;
    VK_CHECK(vmaFindMemoryTypeIndexForImageInfo(
        vmaAllocator,
        &vkInfo,
        &allocInfo,
        &memType));

    VmaPoolCreateInfo poolInfo{};
    poolInfo.memoryTypeIndex = memType;

    VmaPool vmaPool;
    VK_CHECK(vmaCreatePool(vmaAllocator, &poolInfo, &vmaPool));

    pool = UniqueVmaPool{vmaPool, VmaPoolDeleter{vmaAllocator}};

    return vmaPool;
}

auto RenderTargetAllocator::createImage(
    const vk::ImageCreateInfo &info,
    PoolKind                   kind) -> UniqueImage
{
    assert(vmaAllocator != nullptr);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.pool  = ensurePool(kind, info);

    VkImage           vkImg = VK_NULL_HANDLE;
    VmaAllocation     vmaAlloc{};
    VmaAllocationInfo vmaAllocInfo{};

    const VkImageCreateInfo vkInfo = *info;

    VK_CHECK(vmaCreateImage(
        vmaAllocator,
        &vkInfo,
        &allocInfo,
        &vkImg,
        &vmaAlloc,
        &vmaAllocInfo));

    return makeUniqueImage(vmaAllocator, vkImg, vmaAlloc);
}

auto ColorTarget::recreate(
    Device                &device,
    RenderTargetAllocator &alloc,
    vk::Extent2D           extent,
    vk::Format             swapchainFormat) -> void
{
    format = ldrFormatFromSwapchain(swapchainFormat);

    const vk::ImageCreateInfo info{
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D{extent.width, extent.height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled,
    };

    image = alloc.createImage(info, RenderTargetAllocator::PoolKind::kColor);

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.get(),
            vk::ImageViewType::e2D,
            format,
            {},
            range(),
        });
}

auto ColorTarget::range() const -> vk::ImageSubresourceRange
{
    return vk::ImageSubresourceRange{
        vk::ImageAspectFlagBits::eColor,
        0,
        1,
        0,
        1,
    };
}

auto DepthTarget::recreate(
    Device                &device,
    RenderTargetAllocator &alloc,
    vk::Extent2D           extent,
    vk::Format             depthFormat) -> void
{
    format = depthFormat;

    const vk::ImageCreateInfo info{
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D{extent.width, extent.height, 1},
        1,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
    };

    image = alloc.createImage(info, RenderTargetAllocator::PoolKind::kDepth);

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.get(),
            vk::ImageViewType::e2D,
            format,
            {},
            range(),
        });
}

auto DepthTarget::range() const -> vk::ImageSubresourceRange
{
    const auto aspect = depthAspectMask(format);

    return vk::ImageSubresourceRange{
        aspect,
        0,
        1,
        0,
        1,
    };
}

RenderTargets::RenderTargets(
    Device      &device,
    VmaAllocator vma,
    vk::Extent2D extent_,
    vk::Format   swapchainFormat,
    vk::Format   depthFormat)
    : allocator{vma}
{
    recreate(device, extent_, swapchainFormat, depthFormat);
}

auto RenderTargets::recreate(
    Device      &device,
    vk::Extent2D extent_,
    vk::Format   swapchainFormat,
    vk::Format   depthFormat) -> void
{
    extent = extent_;

    sceneColorLdr.recreate(device, allocator, extent_, swapchainFormat);

    mainDepth.recreate(device, allocator, extent_, depthFormat);
}

auto RenderTargets::images() const -> std::vector<vk::Image>
{
    return std::vector<vk::Image>{sceneColorLdr.image.get(), mainDepth.image.get()};
}
