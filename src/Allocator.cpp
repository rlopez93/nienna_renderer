#define VMA_IMPLEMENTATION
#include "Allocator.hpp"
#include "Utility.hpp"

Allocator::Allocator(
    vk::raii::Instance       &instance,
    vk::raii::PhysicalDevice &physicalDevice,
    vk::raii::Device         &device,
    uint32_t                  api)
    : device{device}
{
    // Initialization of VMA allocator.
    // #TODO : VK_EXT_memory_priority ? VMA_ALLOCATOR_CREATE_EXT_MEMORY_PRIORITY_BIT
    VmaAllocatorCreateInfo allocatorCreateInfo{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT
               | VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT,
        .physicalDevice   = *physicalDevice,
        .device           = *device,
        .instance         = *instance,
        .vulkanApiVersion = api};

    allocatorCreateInfo.flags |=
        VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // allow querying for the
                                                        // GPU address of a buffer
    allocatorCreateInfo.flags |= VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE4_BIT;
    allocatorCreateInfo.flags |=
        VMA_ALLOCATOR_CREATE_KHR_MAINTENANCE5_BIT; // allow using
                                                   // VkBufferUsageFlags2CreateInfoKHR

    // Because we use VMA_DYNAMIC_VULKAN_FUNCTIONS
    const VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr   = vkGetDeviceProcAddr,
    };
    allocatorCreateInfo.pVulkanFunctions = &functions;
    VK_CHECK(vmaCreateAllocator(&allocatorCreateInfo, &allocator));
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
    vk::DeviceSize           bufferSize,
    vk::BufferUsageFlags2    usageFlags,
    VmaMemoryUsage           memoryUsage,
    VmaAllocationCreateFlags allocationFlags) const -> Buffer
{

    const auto bufferUsageFlags2CreateInfo = *vk::BufferUsageFlags2CreateInfo{
        usageFlags | vk::BufferUsageFlagBits2::eShaderDeviceAddress};
    auto bufferCreateInfo =
        *vk::BufferCreateInfo{{}, bufferSize, {}, vk::SharingMode::eExclusive};

    bufferCreateInfo.pNext = &bufferUsageFlags2CreateInfo;

    auto vmaAllocationCreateInfo =
        VmaAllocationCreateInfo{.flags = allocationFlags, .usage = memoryUsage};

    // const VkDeviceSize dedicatedMemoryMinSize = 64ULL * 1024; // 64 KB
    //
    // if (bufferSize > dedicatedMemoryMinSize) {
    //     vmaAllocationCreateInfo.flags |=
    //         VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT; // use dedicated memory for
    //                                                     // large buffers
    // }

    // VkBufferCreateInfo b_info         = *bufferCreateInfo;
    auto allocationInfo = VmaAllocationInfo{};

    // fmt::print(stderr, "\n\ncalling vmaCreateBuffer()...\n\n");
    Buffer   buffer;
    VkBuffer vk_buffer;
    VK_CHECK(vmaCreateBuffer(
        allocator,
        &bufferCreateInfo,
        &vmaAllocationCreateInfo,
        &vk_buffer,
        &buffer.allocation,
        &allocationInfo));

    buffer.buffer = vk_buffer;

    return buffer;
}
//*-- Destroy a buffer -*/
void Allocator::destroyBuffer(Buffer buffer) const
{
    vmaDestroyBuffer(allocator, buffer.buffer, buffer.allocation);
}

/*--
 * Create an image in GPU memory. This does not adding data to the image.
 * This is only creating the image in GPU memory.
 * See createImageAndUploadData for creating an image and uploading data.
-*/
[[nodiscard]]
auto Allocator::createImage(const vk::ImageCreateInfo &imageInfo) const -> Image
{
    const auto imageCreateInfo = *imageInfo;

    const VmaAllocationCreateInfo createInfo{.usage = VMA_MEMORY_USAGE_GPU_ONLY};

    Image             image;
    VkImage           vk_image;
    VmaAllocationInfo allocInfo{};
    VK_CHECK(vmaCreateImage(
        allocator,
        &imageCreateInfo,
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
