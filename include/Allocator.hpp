#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#define VMA_USE_STL_CONTAINERS 1
#include <vma/vk_mem_alloc.h>
#include <vulkan/vulkan_raii.hpp>

#ifdef NDEBUG
#define VK_CHECK(vkFnc) vkFnc
#else
#include <cassert>
#include <fmt/base.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_CHECK(vkFnc)                                                                \
    {                                                                                  \
        if (const VkResult checkResult = (vkFnc); checkResult != VK_SUCCESS) {         \
            const char *errMsg = string_VkResult(checkResult);                         \
            fmt::print(stderr, "Vulkan error: {}", errMsg);                            \
            assert(checkResult == VK_SUCCESS);                                         \
        }                                                                              \
    }
#endif

struct Buffer {
    vk::Buffer        buffer{};
    VmaAllocation     allocation{};
    vk::DeviceAddress address{};
};

struct Image {
    vk::Image     image{};
    VmaAllocation allocation{};
};

struct Allocator {
    Allocator(VmaAllocatorCreateInfo allocatorInfo);

    [[nodiscard]]
    auto createBuffer(
        vk::DeviceSize           deviceSize,
        vk::BufferUsageFlags2    usage,
        VmaMemoryUsage           memoryUsage = VMA_MEMORY_USAGE_AUTO,
        VmaAllocationCreateFlags flags       = {}) const -> Buffer;

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

    void freeStagingBuffers();

    vk::Device          device;
    VmaAllocator        allocator;
    std::vector<Buffer> stagingBuffers;
};
