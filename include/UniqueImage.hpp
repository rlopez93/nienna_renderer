#pragma once

#include <memory>

#include <vulkan/vulkan.hpp>

#include "vma.hpp"

struct VmaImageDeleter {
    VmaAllocator  allocator  = nullptr;
    VmaAllocation allocation = nullptr;

    auto operator()(VkImage_T *img) const noexcept -> void;
};

using UniqueImage = std::unique_ptr<VkImage_T, VmaImageDeleter>;

auto makeUniqueImage(
    VmaAllocator  allocator,
    vk::Image     image,
    VmaAllocation allocation) -> UniqueImage;
