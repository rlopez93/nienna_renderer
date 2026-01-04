#pragma once

#include <vulkan/vulkan_raii.hpp>

void cmdBarrierUndefinedToTransferDst(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range);

void cmdBarrierTransferDstToShaderReadOnly(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range);

void cmdBarrierUndefinedToDepthAttachment(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range);

void cmdBarrierSwapchainToColorAttachment(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageLayout           oldLayout,
    vk::ImageSubresourceRange range);

void cmdBarrierColorAttachmentToPresent(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range);
