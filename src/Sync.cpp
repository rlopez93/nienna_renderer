// src/Sync.cpp
#include "Sync.hpp"

namespace
{

auto submitImageBarrier(
    vk::raii::CommandBuffer &cmd,
    vk::ImageMemoryBarrier2  barrier) -> void
{
    cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));
}

} // namespace

void cmdBarrierUndefinedToTransferDst(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range)
{
    submitImageBarrier(
        cmd,
        vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eTransferDstOptimal,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            range});
}

void cmdBarrierTransferDstToShaderReadOnly(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range)
{
    submitImageBarrier(
        cmd,
        vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite,
            vk::PipelineStageFlagBits2::eFragmentShader
                | vk::PipelineStageFlagBits2::eComputeShader,
            vk::AccessFlagBits2::eShaderSampledRead,
            vk::ImageLayout::eTransferDstOptimal,
            vk::ImageLayout::eShaderReadOnlyOptimal,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            range});
}

void cmdBarrierUndefinedToDepthAttachment(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range)
{
    submitImageBarrier(
        cmd,
        vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eEarlyFragmentTests
                | vk::PipelineStageFlagBits2::eLateFragmentTests,
            vk::AccessFlagBits2::eDepthStencilAttachmentRead
                | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eDepthAttachmentOptimal,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            range});
}

void cmdBarrierSwapchainToColorAttachment(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageLayout           oldLayout,
    vk::ImageSubresourceRange range)
{
    submitImageBarrier(
        cmd,
        vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone,
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentRead
                | vk::AccessFlagBits2::eColorAttachmentWrite,
            oldLayout,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            range});
}

void cmdBarrierColorAttachmentToPresent(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range)
{
    submitImageBarrier(
        cmd,
        vk::ImageMemoryBarrier2{
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentWrite,
            vk::PipelineStageFlagBits2::eNone,
            vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::QueueFamilyIgnored,
            vk::QueueFamilyIgnored,
            image,
            range});
}
