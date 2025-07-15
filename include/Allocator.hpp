#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#include "Utility.hpp"

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
    vk::raii::ImageView view;     // Image view
    vk::Extent2D        extent{}; // Size of the image
    vk::ImageLayout
        layout{}; // Layout of the image (color attachment, shader read, ...)
};

struct Allocator {
    Allocator(
        vk::raii::Instance       &instance,
        vk::raii::PhysicalDevice &physicalDevice,
        vk::raii::Device         &device,
        uint32_t                  api);

    ~Allocator()
    {
        vmaDestroyAllocator(allocator);
    }

    [[nodiscard]]
    auto createBuffer(
        vk::DeviceSize           deviceSize,
        vk::BufferUsageFlags2    usage,
        VmaMemoryUsage           memoryUsage = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlags flags       = {}) const -> Buffer;

    void destroyBuffer(Buffer buffer) const;

    /*--
     * Create a staging buffer, copy data into it, and track it.
     * This method accepts data, handles the mapping, copying, and unmapping
     * automatically.
    -*/
    template <typename T>
    [[nodiscard]]
    auto createStagingBuffer(const std::vector<T> &vectorData) -> Buffer
    {
        const vk::DeviceSize bufferSize = sizeof(T) * vectorData.size();

        fmt::print(stderr, "size {}\n", vectorData.size() * sizeof(T));

        fmt::print(stderr, "\n\ncalling createBuffer()...\n\n");
        // Create a staging buffer
        Buffer stagingBuffer = createBuffer(
            bufferSize,
            vk::BufferUsageFlagBits2::eTransferSrc,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
                | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        // Track the staging buffer for later cleanup
        stagingBuffers.push_back(stagingBuffer);

        // fmt::print(stderr, "\n\ncalling vmaCopyMemoryToAllocation()...\n\n");
        vmaCopyMemoryToAllocation(
            allocator,
            vectorData.data(),
            stagingBuffer.allocation,
            0,
            bufferSize);

        // Map and copy data to the staging buffer
        // void *data;
        // vmaMapMemory(allocator, stagingBuffer.allocation, &data);
        // memcpy(data, vectorData.data(), (size_t)bufferSize);
        // vmaUnmapMemory(allocator, stagingBuffer.allocation);
        return stagingBuffer;
    }

    /*--
     * Create a buffer (GPU only) with data, this is done using a staging buffer
     * The staging buffer is a buffer that is used to transfer data from the CPU
     * to the GPU.
     * and cannot be freed until the data is transferred. So the command buffer
     * must be submitted, then
     * the staging buffer can be cleared using the freeStagingBuffers function.
    -*/
    template <typename T>
    [[nodiscard]]
    auto createBufferAndUploadData(
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

    [[nodiscard]]
    auto createImage(const vk::ImageCreateInfo &imageInfo) const -> Image;

    void destroyImage(Image &image) const;

    /*-- Create an image and upload data using a staging buffer --*/
    template <typename T>
    [[nodiscard]]
    auto createImageAndUploadData(
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
            vk::ImageLayout::eTransferDstOptimal
            // VK_IMAGE_LAYOUT_UNDEFINED,
            // VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
        );

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

    void freeStagingBuffers();

    vk::Device          device;
    VmaAllocator        allocator;
    std::vector<Buffer> stagingBuffers;
};
