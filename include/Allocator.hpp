#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

/*--
 * A buffer is a region of memory used to store data.
 * It is used to store vertex data, index data, uniform data, and other types of data.
 * There is a VkBuffer object that represents the buffer, and a VmaAllocation object
that represents the memory allocation.
 * The address is used to access the buffer in the shader.
-*/
struct Buffer {
    vk::Buffer        buffer{};
    VmaAllocation     allocation{};
    vk::DeviceAddress address{};
};

/*--
 * The image resource is an image with an image view and a layout.
 * and other information like format and extent.
-*/
struct Image {
    vk::Image     image{};
    VmaAllocation allocation{};
};

/*The image resource is an image with an image view and a
        layout and other information like format and extent*/
struct ImageResource : Image {
    vk::ImageView view{};   // Image view
    vk::Extent2D  extent{}; // Size of the image
    vk::ImageLayout
        layout{}; // Layout of the image (color attachment, shader read, ...)
};

struct Allocator {
    Allocator(VmaAllocatorCreateInfo allocatorInfo);

    [[nodiscard]]
    auto createBuffer(
        vk::DeviceSize           deviceSize,
        vk::BufferUsageFlags2    usage,
        VmaMemoryUsage           memoryUsage = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlags flags       = {}) const -> Buffer;

    void destroyBuffer(Buffer buffer) const;

    template <typename T>
    [[nodiscard]]
    auto createStagingBuffer(const std::span<T> &vectorData) -> Buffer;

    template <typename T>
    [[nodiscard]]
    auto createBufferAndUploadData(
        vk::raii::CommandBuffer &cmd,
        const std::span<T>      &vectorData,
        vk::BufferUsageFlags2    usageFlags) -> Buffer;

    [[nodiscard]]
    auto createImage(const vk::ImageCreateInfo &imageInfo) const -> Image;

    void destroyImage(Image &image) const;

    void destroyImageResource(ImageResource &imageResource) const;

    template <typename T>
    [[nodiscard]]
    auto createImageAndUploadData(
        vk::raii::CommandBuffer   &cmd,
        const std::span<T>        &vectorData,
        const vk::ImageCreateInfo &_imageInfo,
        vk::ImageLayout            finalLayout) -> ImageResource;
    void freeStagingBuffers();

    vk::Device          device;
    VmaAllocator        allocator;
    std::vector<Buffer> stagingBuffers;
};
