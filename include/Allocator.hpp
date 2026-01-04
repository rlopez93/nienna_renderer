#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "Command.hpp"
#include "vma.hpp"

#include "Buffer.hpp"
#include "Device.hpp"
#include "Image.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Sync.hpp"

struct Allocator {
    Allocator(
        Instance       &instance,
        PhysicalDevice &physicalDevice,
        Device         &device);

    ~Allocator();

    [[nodiscard]]
    auto handle() const -> VmaAllocator
    {
        return allocator.get();
    }

    [[nodiscard]]
    auto handle() -> VmaAllocator
    {
        return allocator.get();
    }

    [[nodiscard]]
    auto createBuffer(
        vk::DeviceSize           deviceSize,
        vk::BufferUsageFlags2    usage,
        bool                     isStagingBuffer = false,
        VmaMemoryUsage           memoryUsage     = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlags flags           = {}) -> Buffer;

    void destroyBuffer(Buffer buffer) const;

    template <typename T>
    [[nodiscard]]
    auto createStagingBuffer(const std::vector<T> &vectorData) -> Buffer;

    template <typename T>
    [[nodiscard]]
    auto createBufferAndUploadData(
        Command              &cmd,
        const std::vector<T> &vectorData,
        vk::BufferUsageFlags2 usageFlags) -> Buffer;

    [[nodiscard]]
    auto createImage(const vk::ImageCreateInfo &imageInfo) -> Image;

    void destroyImage(Image image) const;

    template <typename T>
    [[nodiscard]]
    auto createImageAndUploadData(
        Command              &cmd,
        const std::vector<T> &vectorData,
        vk::ImageCreateInfo   imageInfo,
        vk::ImageLayout       finalLayout) -> Image;

    void freeStagingBuffers();
    void freeBuffers();
    void freeImages();

  private:
    struct VmaAllocatorDeleter {
        VmaAllocatorDeleter() = default;
        auto operator()(VmaAllocator_T *allocator) const -> void
        {
            vmaDestroyAllocator(allocator);
        }
    };

    using UniqueVmaAllocator = std::unique_ptr<VmaAllocator_T, VmaAllocatorDeleter>;

    static auto createUniqueVmaAllocator(
        Instance       &instance,
        PhysicalDevice &physicalDevice,
        Device         &device) -> UniqueVmaAllocator;

    UniqueVmaAllocator  allocator;
    vk::Device          device;
    std::vector<Buffer> stagingBuffers;
    std::vector<Buffer> buffers;
    std::vector<Image>  images;
};

template <typename T>
[[nodiscard]]
auto Allocator::createImageAndUploadData(
    Command              &cmd,
    const std::vector<T> &vectorData,
    vk::ImageCreateInfo   imageInfo,
    vk::ImageLayout       finalLayout) -> Image
{
    assert(finalLayout == vk::ImageLayout::eShaderReadOnlyOptimal);

    Buffer staging = createStagingBuffer(vectorData);

    imageInfo.setUsage(imageInfo.usage | vk::ImageUsageFlagBits::eTransferDst);

    Image image = createImage(imageInfo);

    vk::ImageSubresourceRange range{
        vk::ImageAspectFlagBits::eColor,
        0,
        imageInfo.mipLevels,
        0,
        imageInfo.arrayLayers};

    cmdBarrierUndefinedToTransferDst(cmd.buffer, image.image, range);

    cmd.buffer.copyBufferToImage(
        staging.buffer,
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{
            {},
            {},
            {},
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {},
            imageInfo.extent});

    cmdBarrierTransferDstToShaderReadOnly(cmd.buffer, image.image, range);

    return image;
}
template <typename T>
inline auto Allocator::createBufferAndUploadData(
    Command              &cmd,
    const std::vector<T> &vectorData,
    vk::BufferUsageFlags2 usageFlags) -> Buffer
{
    // fmt::print(stderr, "\n\ncalling createStagingBuffer()...\n\n");
    // Create staging buffer and upload data
    Buffer stagingBuffer = createStagingBuffer(vectorData);

    // Create the final buffer in GPU memory
    const vk::DeviceSize bufferSize = sizeof(T) * vectorData.size();

    // fmt::print(stderr, "\n\ncalling createBuffer()...\n\n");
    Buffer buffer = createBuffer(
        bufferSize,
        usageFlags | vk::BufferUsageFlagBits2::eTransferDst,
        VMA_MEMORY_USAGE_GPU_ONLY);

    // fmt::print(stderr, "\n\ncalling cmd.copyBuffer()...\n\n");
    cmd.buffer.copyBuffer(
        stagingBuffer.buffer,
        buffer.buffer,
        vk::BufferCopy{}.setSize(bufferSize));

    // fmt::print(stderr, "\n\nexiting...\n\n");
    return buffer;
}

template <typename T>
inline auto Allocator::createStagingBuffer(const std::vector<T> &vectorData) -> Buffer
{
    const vk::DeviceSize bufferSize = sizeof(T) * vectorData.size();

    // fmt::println(stderr, "size {}", vectorData.size() * sizeof(T));

    // fmt::println(stderr, "calling createBuffer()...");
    // Create a staging buffer
    Buffer stagingBuffer = createBuffer(
        bufferSize,
        vk::BufferUsageFlagBits2::eTransferSrc,
        true,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_MAPPED_BIT
            | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    // fmt::print(stderr, "\n\ncalling vmaCopyMemoryToAllocation()...\n\n");
    vmaCopyMemoryToAllocation(
        allocator.get(),
        vectorData.data(),
        stagingBuffer.allocation,
        0,
        bufferSize);

    return stagingBuffer;
}
