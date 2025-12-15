#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>

#include <vulkan/vulkan_raii.hpp>

#include "Device.hpp"
#include "Instance.hpp"
#include "PhysicalDevice.hpp"
#include "Utility.hpp"

struct Buffer {
    vk::Buffer    buffer{};
    VmaAllocation allocation{};
};

struct Image {
    vk::Image       image{};
    VmaAllocation   allocation{};
    vk::ImageLayout currentLayout;
};

struct Allocator {
    Allocator(
        Instance       &instance,
        PhysicalDevice &physicalDevice,
        Device         &device);

    ~Allocator();

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
        vk::raii::CommandBuffer &cmd,
        const std::vector<T>    &vectorData,
        vk::BufferUsageFlags2    usageFlags) -> Buffer;

    [[nodiscard]]
    auto createImage(const vk::ImageCreateInfo &imageInfo) -> Image;

    void destroyImage(Image image) const;

    template <typename T>
    [[nodiscard]]
    auto createImageAndUploadData(
        vk::raii::CommandBuffer &cmd,
        const std::vector<T>    &vectorData,
        vk::ImageCreateInfo      imageInfo,
        vk::ImageLayout          finalLayout) -> Image;

    void freeStagingBuffers();
    void freeBuffers();
    void freeImages();

    vk::Device          device;
    VmaAllocator        allocator;
    std::vector<Buffer> stagingBuffers;
    std::vector<Buffer> buffers;
    std::vector<Image>  images;
};

template <typename T>
inline auto Allocator::createImageAndUploadData(
    vk::raii::CommandBuffer &cmd,
    const std::vector<T>    &vectorData,
    vk::ImageCreateInfo      imageInfo,
    vk::ImageLayout          finalLayout) -> Image
{
    // Create staging buffer and upload data
    Buffer stagingBuffer = createStagingBuffer(vectorData);

    // Create image in GPU memory
    imageInfo.setUsage(imageInfo.usage | vk::ImageUsageFlagBits::eTransferDst);
    Image image = createImage(imageInfo);

    // Transition image layout for copying data
    cmdTransitionImageLayout(
        cmd,
        image.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eTransferDstOptimal);

    // Copy buffer data to the image
    cmd.copyBufferToImage(
        stagingBuffer.buffer,
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        vk::BufferImageCopy{
            {},
            {},
            {},
            vk::ImageSubresourceLayers{vk::ImageAspectFlagBits::eColor, 0, 0, 1},
            {},
            imageInfo.extent});

    // Transition image layout to final layout
    cmdTransitionImageLayout(
        cmd,
        image.image,
        vk::ImageLayout::eTransferDstOptimal,
        finalLayout);

    return image;
}

template <typename T>
inline auto Allocator::createBufferAndUploadData(
    vk::raii::CommandBuffer &cmd,
    const std::vector<T>    &vectorData,
    vk::BufferUsageFlags2    usageFlags) -> Buffer
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
    cmd.copyBuffer(
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
        allocator,
        vectorData.data(),
        stagingBuffer.allocation,
        0,
        bufferSize);

    return stagingBuffer;
}
