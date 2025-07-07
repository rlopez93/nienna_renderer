#define VMA_IMPLEMENTATION
#include "Allocator.hpp"

Allocator::Allocator(VmaAllocatorCreateInfo allocatorInfo)
{
    // Initialization of VMA allocator.
    // #TODO : VK_EXT_memory_priority ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT

    allocatorInfo.flags |=
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // allow querying for the
                                                        // GPU address of a buffer
    allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    allocatorInfo.flags |=
        VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT; // allow using
                                                   // VkBufferUsageFlags2CreateInfoKHR

    device = allocatorInfo.device;
    // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };
    allocatorInfo.pVulkanFunctions = &functions;
    vmaCreateAllocator(&allocatorInfo, &allocator);
}

/*-- Create a buffer -*/
/*
 * UBO: VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
 *        + VMA_MEMORY_USAGE_CPU_TO_GPU
 * SSBO: VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
 *        + VMA_MEMORY_USAGE_CPU_TO_GPU // Use this if the CPU will frequently
 * update the buffer
 *        + VMA_MEMORY_USAGE_GPU_ONLY // Use this if the CPU will rarely update the
 * buffer
 *        + VMA_MEMORY_USAGE_GPU_TO_CPU  // Use this when you need to read back data
 * from the SSBO to the CPU
 *      ----
 *        + VMA_ALLOCATION_CREATE_MAPPED_BIT // Automatically maps the buffer upon
 * creation
 *        + VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT // If the CPU
 * will sequentially write to the buffer's memory,
 */
[[nodiscard]]
auto Allocator::createBuffer(
    vk::DeviceSize           deviceSize,
    vk::BufferUsageFlags2    usage,
    VmaMemoryUsage           memoryUsage,
    VmaAllocationCreateFlags flags) const -> Buffer
{

    const auto bufferUsageFlags2CreateInfo = vk::BufferUsageFlags2CreateInfo{
        usage | vk::BufferUsageFlagBits2::eShaderDeviceAddress};
    const auto bufferCreateInfo =
        vk::BufferCreateInfo{{}, deviceSize, {}, vk::SharingMode::eExclusive}.setPNext(
            &bufferUsageFlags2CreateInfo);

    auto vmaAllocationCreateInfo =
        VmaAllocationCreateInfo{.flags = flags, .usage = memoryUsage};
    const VkDeviceSize dedicatedMemoryMinSize = 64ULL * 1024; // 64 KB

    if (deviceSize > dedicatedMemoryMinSize) {
        vmaAllocationCreateInfo.flags |=
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // use dedicated memory for
                                                        // large buffers
    }
    auto allocationInfo = VmaAllocationInfo{};

    Buffer   buffer;
    VkBuffer vk_buffer;
    vmaCreateBuffer(
        allocator,
        &*bufferCreateInfo,
        &vmaAllocationCreateInfo,
        &vk_buffer,
        &buffer.allocation,
        &allocationInfo);

    buffer.buffer = vk_buffer;

    return buffer;
}

/*--
 * Create a staging buffer, copy data into it, and track it.
 * This method accepts data, handles the mapping, copying, and unmapping
 * automatically.
-*/
template <typename T>
[[nodiscard]]
auto Allocator::createStagingBuffer(const std::span<T> &vectorData) -> Buffer
{
    const vk::DeviceSize bufferSize = sizeof(T) * vectorData.size();

    // Create a staging buffer
    Buffer stagingBuffer = createBuffer(
        bufferSize,
        // VK_BUFFER_USAGE_2_TRANSFER_SRC_BIT_KHR,
        vk::BufferUsageFlagBits2::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    // Track the staging buffer for later cleanup
    stagingBuffers.push_back(stagingBuffer);

    // Map and copy data to the staging buffer
    void *data;
    vmaMapMemory(allocator, stagingBuffer.allocation, &data);
    memcpy(data, vectorData.data(), (size_t)bufferSize);
    vmaUnmapMemory(allocator, stagingBuffer.allocation);
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
auto Allocator::createBufferAndUploadData(
    vk::raii::CommandBuffer &cmd,
    const std::span<T>      &vectorData,
    vk::BufferUsageFlags2    usageFlags) -> Buffer
{
    // Create staging buffer and upload data
    Buffer stagingBuffer = createStagingBuffer(vectorData);

    // Create the final buffer in GPU memory
    const vk::DeviceSize bufferSize = sizeof(T) * vectorData.size();

    Buffer buffer = createBuffer(
        bufferSize,
        usageFlags | vk::BufferUsageFlagBits2::eTransferDst,
        VMA_MEMORY_USAGE_GPU_ONLY);

    cmd.copyBuffer(
        stagingBuffer.buffer,
        buffer.buffer,
        vk::BufferCopy{}.setSize(bufferSize));

    return buffer;
}

/*--
 * Create an image in GPU memory. This does not adding data to the image.
 * This is only creating the image in GPU memory.
 * See createImageAndUploadData for creating an image and uploading data.
-*/
[[nodiscard]]
auto Allocator::createImage(const vk::ImageCreateInfo &imageInfo) const -> Image
{
    const VmaAllocationCreateInfo createInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY};

    Image             image;
    VkImage           vk_image;
    VmaAllocationInfo allocInfo{};
    VK_CHECK(vmaCreateImage(
        allocator,
        &*imageInfo,
        &createInfo,
        &vk_image,
        &image.allocation,
        &allocInfo));
    image.image = vk_image;
    return image;
}

/*-- Destroy image --*/
void Allocator::destroyImage(Image &image) const
{
    vmaDestroyImage(allocator, image.image, image.allocation);
}

void destroyImageResource(ImageResource &imageRessource) const
{
    destroyImage(imageRessource);
    vkDestroyImageView(m_device, imageRessource.view, nullptr);
}

/*-- Create an image and upload data using a staging buffer --*/
template <typename T>
[[nodiscard]]
ImageResource createImageAndUploadData(
    VkCommandBuffer          cmd,
    const std::span<T>      &vectorData,
    const VkImageCreateInfo &_imageInfo,
    VkImageLayout            finalLayout)
{
    // Create staging buffer and upload data
    Buffer stagingBuffer = createStagingBuffer(vectorData);

    // Create image in GPU memory
    VkImageCreateInfo imageInfo = _imageInfo;
    imageInfo.usage |=
        VK_IMAGE_USAGE_TRANSFER_DST_BIT; // We will copy data to this image
    Image image = createImage(imageInfo);

    // Transition image layout for copying data
    cmdTransitionImageLayout(
        cmd,
        image.image,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy buffer data to the image
    const std::array<VkBufferImageCopy, 1> copyRegion{
        {{.imageSubresource =
              {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1},
          .imageExtent = imageInfo.extent}}};

    vkCmdCopyBufferToImage(
        cmd,
        stagingBuffer.buffer,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        uint32_t(copyRegion.size()),
        copyRegion.data());

    // Transition image layout to final layout
    cmdTransitionImageLayout(
        cmd,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        finalLayout);

    ImageResource resultImage(image);
    resultImage.layout = finalLayout;
    return resultImage;
}

/*--
 * The staging buffers are buffers that are used to transfer data from the CPU to
the GPU.
 * They cannot be freed until the data is transferred. So the command buffer must be
completed, then the staging buffer can be cleared.
-*/
void Allocator::freeStagingBuffers()
{
    for (const auto &buffer : stagingBuffers) {
        destroyBuffer(buffer);
    }
    stagingBuffers.clear();
}
