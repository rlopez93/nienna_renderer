// ==> src/DepthTarget.cpp <==
#include "DepthTarget.hpp"

#include "Allocator.hpp"
#include "Command.hpp"
#include "Device.hpp"
#include "Sync.hpp"

#include <cassert>

namespace
{

[[nodiscard]]
auto depthAspectMask(vk::Format fmt) -> vk::ImageAspectFlags
{
    switch (fmt) {
    case vk::Format::eD32SfloatS8Uint:
    case vk::Format::eD24UnormS8Uint:
        return vk::ImageAspectFlagBits::eDepth | vk::ImageAspectFlagBits::eStencil;
    default:
        return vk::ImageAspectFlagBits::eDepth;
    }
}

} // namespace

DepthTarget::DepthTarget(
    Device      &device,
    Allocator   &allocator,
    Command     &command,
    vk::Extent2D extent,
    vk::Format   depthFormat)
{
    recreate(device, allocator, command, extent, depthFormat);
}

void DepthTarget::recreate(
    Device      &device,
    Allocator   &allocator,
    Command     &command,
    vk::Extent2D extent,
    vk::Format   depthFormat)
{
    assert(depthFormat != vk::Format::eUndefined);

    format = depthFormat;

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

    const auto aspect = depthAspectMask(format);

    const auto range = vk::ImageSubresourceRange{aspect, 0, 1, 0, 1};

    view = device.handle.createImageView(
        vk::ImageViewCreateInfo{
            {},
            image.image,
            vk::ImageViewType::e2D,
            format,
            {},
            range});

    command.beginSingleTime();

    cmdBarrierUndefinedToDepthAttachment(command.buffer, image.image, range);

    command.endSingleTime(device);
}
