#pragma once

#include "vma.hpp"

#include <vulkan/vulkan.hpp>

struct Buffer {
    vk::Buffer    buffer{};
    VmaAllocation allocation{};
};
