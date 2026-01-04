#include "UniqueImage.hpp"

auto VmaImageDeleter::operator()(VkImage_T *img) const noexcept -> void
{
    if (!allocator || !img) {
        return;
    }
    vmaDestroyImage(allocator, img, allocation);
}

auto makeUniqueImage(
    VmaAllocator  allocator,
    vk::Image     image,
    VmaAllocation allocation) -> UniqueImage
{
    return UniqueImage{image, VmaImageDeleter{allocator, allocation}};
}
