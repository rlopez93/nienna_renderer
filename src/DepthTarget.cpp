#include "DepthTarget.hpp"
#include "Allocator.hpp"
#include "Command.hpp"
#include "Device.hpp"

void DepthTarget::recreate(
    Device      &device,
    Allocator   &allocator,
    Command     &command,
    vk::Extent2D extent)
{
    // Destroy old resources by overwriting RAII handles

    image = allocator.createImage(
        vk::ImageCreateInfo{
            {},
            vk::ImageType::e2D,
            format,
            vk::Extent3D{extent.width, extent.height, 1},
            1,
            1,
            vk::SampleCountFlagBits::e1,
            vk::ImageTiling::eOptimal,
            vk::ImageUsageFlagBits::eDepthStencilAttachment});

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.image,
            vk::ImageViewType::e2D,
            format,
            {},
            vk::ImageSubresourceRange{vk::ImageAspectFlagBits::eDepth, 0, 1, 0, 1}});

    // Transition layout once
    command.beginSingleTime();
    cmdTransitionImageLayout(
        command.buffer,
        image.image,
        vk::ImageLayout::eUndefined,
        vk::ImageLayout::eAttachmentOptimal,
        vk::ImageAspectFlagBits::eDepth);
    command.endSingleTime(device);
}
