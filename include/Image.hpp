#pragma once

#include "vma.hpp"

#include <vulkan/vulkan.hpp>

struct Image {
    vk::Image     image;
    VmaAllocation allocation;
};
