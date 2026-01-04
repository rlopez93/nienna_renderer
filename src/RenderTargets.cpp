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
    : allocator{vma}
{
}

RenderTargetAllocator::~RenderTargetAllocator()
{
    destroyPool(colorPool);
    destroyPool(depthPool);
}

auto RenderTargetAllocator::vma() const -> VmaAllocator
{
    return allocator;
}

auto RenderTargetAllocator::setAllocator(VmaAllocator vma) -> void
{
    allocator = vma;
}

auto RenderTargetAllocator::destroyPool(VmaPool &pool) -> void
{
    if (allocator == nullptr || pool == nullptr) {
        pool = nullptr;
        return;
    }

    vmaDestroyPool(allocator, pool);
    pool = nullptr;
}

auto RenderTargetAllocator::ensurePool(
    PoolKind                   kind,
    const vk::ImageCreateInfo &info) -> VmaPool
{
    assert(allocator != nullptr);

    VmaPool &pool = (kind == PoolKind::kColor) ? colorPool : depthPool;

    if (pool != nullptr) {
        return pool;
    }

    const VkImageCreateInfo vkInfo = *info;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    std::uint32_t memType = 0u;
    VK_CHECK(
        vmaFindMemoryTypeIndexForImageInfo(allocator, &vkInfo, &allocInfo, &memType));

    VmaPoolCreateInfo poolInfo{};
    poolInfo.memoryTypeIndex = memType;

    VK_CHECK(vmaCreatePool(allocator, &poolInfo, &pool));

    return pool;
}

auto RenderTargetAllocator::createImage(
    const vk::ImageCreateInfo &info,
    PoolKind                   kind) -> Image
{
    assert(allocator != nullptr);

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    allocInfo.pool  = ensurePool(kind, info);

    VkImage           vkImg = VK_NULL_HANDLE;
    VmaAllocation     vmaAlloc{};
    VmaAllocationInfo vmaAllocInfo{};

    const VkImageCreateInfo vkInfo = *info;

    VK_CHECK(vmaCreateImage(
        allocator,
        &vkInfo,
        &allocInfo,
        &vkImg,
        &vmaAlloc,
        &vmaAllocInfo));

    return Image{
        .image      = vk::Image{vkImg},
        .allocation = vmaAlloc,
    };
}

auto RenderTargetAllocator::destroyImage(Image img) const -> void
{
    if (allocator == nullptr) {
        return;
    }

    if (img.image == vk::Image{} || img.allocation == nullptr) {
        return;
    }

    vmaDestroyImage(allocator, static_cast<VkImage>(img.image), img.allocation);
}

OwnedImage::OwnedImage(
    VmaAllocator allocator_,
    Image        image_)
    : allocator{allocator_},
      image{image_}
{
}

OwnedImage::~OwnedImage()
{
    reset();
}

OwnedImage::OwnedImage(OwnedImage &&other) noexcept
    : allocator{other.allocator},
      image{other.image}
{
    other.allocator = nullptr;
    other.image     = Image{};
}

auto OwnedImage::operator=(OwnedImage &&other) noexcept -> OwnedImage &
{
    if (this == &other) {
        return *this;
    }

    reset();

    allocator = other.allocator;
    image     = other.image;

    other.allocator = nullptr;
    other.image     = Image{};

    return *this;
}

auto OwnedImage::vkImage() const -> vk::Image
{
    return image.image;
}

auto OwnedImage::reset() -> void
{
    if (allocator == nullptr) {
        image = Image{};
        return;
    }

    if (image.image != vk::Image{} && image.allocation != nullptr) {
        vmaDestroyImage(allocator, static_cast<VkImage>(image.image), image.allocation);
    }

    image = Image{};
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

    Image img = alloc.createImage(info, RenderTargetAllocator::PoolKind::kColor);

    image = OwnedImage{alloc.vma(), img};

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.vkImage(),
            vk::ImageViewType::e2D,
            format,
            {},
            range(),
        });
}

auto ColorTarget::vkImage() const -> vk::Image
{
    return image.vkImage();
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

    Image img = alloc.createImage(info, RenderTargetAllocator::PoolKind::kDepth);

    image = OwnedImage{alloc.vma(), img};

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.vkImage(),
            vk::ImageViewType::e2D,
            format,
            {},
            range(),
        });
}
auto DepthTarget::vkImage() const -> vk::Image
{
    return image.vkImage();
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
    recreate(device, vma, extent_, swapchainFormat, depthFormat);
}

auto RenderTargets::recreate(
    Device      &device,
    VmaAllocator vma,
    vk::Extent2D extent_,
    vk::Format   swapchainFormat,
    vk::Format   depthFormat) -> void
{
    allocator.setAllocator(vma);
    extent = extent_;

    sceneColorLdr.recreate(device, allocator, extent_, swapchainFormat);

    mainDepth.recreate(device, allocator, extent_, depthFormat);
}

auto RenderTargets::images() const -> std::vector<vk::Image>
{
    return std::vector<vk::Image>{
        sceneColorLdr.vkImage(),
        mainDepth.vkImage(),
    };
}
