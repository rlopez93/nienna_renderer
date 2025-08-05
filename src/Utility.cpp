#include "Utility.hpp"

auto makePipelineStageAccessTuple(vk::ImageLayout state) -> std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2>
{
    switch (state) {
    case vk::ImageLayout::eUndefined:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eTopOfPipe,
            vk::AccessFlagBits2::eNone);
    case vk::ImageLayout::eColorAttachmentOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eColorAttachmentRead
                | vk::AccessFlagBits2::eColorAttachmentWrite);
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eFragmentShader
                | vk::PipelineStageFlagBits2::eComputeShader
                | vk::PipelineStageFlagBits2::ePreRasterizationShaders,
            vk::AccessFlagBits2::eShaderRead);
    case vk::ImageLayout::eTransferDstOptimal:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eTransferWrite);
    case vk::ImageLayout::eGeneral:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eComputeShader
                | vk::PipelineStageFlagBits2::eTransfer,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite
                | vk::AccessFlagBits2::eTransferWrite);
    case vk::ImageLayout::ePresentSrcKHR:
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            vk::AccessFlagBits2::eNone);
    default: {
        assert(false && "Unsupported layout transition!");
        return std::make_tuple(
            vk::PipelineStageFlagBits2::eAllCommands,
            vk::AccessFlagBits2::eMemoryRead | vk::AccessFlagBits2::eMemoryWrite);
    }
    }
}

void cmdTransitionImageLayout(
    vk::raii::CommandBuffer &cmd,
    VkImage                  image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout,
    vk::ImageAspectFlags     aspectMask)

{
    static int64_t count = 0;
    // fmt::print(stderr, "\n\ncmdTransitionImageLayout(): {}\n\n", count);
    ++count;
    const auto [srcStage, srcAccess] = makePipelineStageAccessTuple(oldLayout);
    const auto [dstStage, dstAccess] = makePipelineStageAccessTuple(newLayout);

    auto barrier = vk::ImageMemoryBarrier2{
        srcStage,
        srcAccess,
        dstStage,
        dstAccess,
        oldLayout,
        newLayout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        {aspectMask, 0, 1, 0, 1}};

    cmd.pipelineBarrier2(vk::DependencyInfo{{}, {}, {}, barrier});
}

void cmdBufferMemoryBarrier(
    vk::raii::CommandBuffer &commandBuffer,
    vk::Buffer              &buffer,
    vk::PipelineStageFlags2  srcStageMask,
    vk::PipelineStageFlags2  dstStageMask,
    vk::AccessFlags2         srcAccessMask, // Default to infer if not provided
    vk::AccessFlags2         dstAccessMask, // Default to infer if not provided
    vk::DeviceSize           offset,
    vk::DeviceSize           size,
    uint32_t                 srcQueueFamilyIndex,
    uint32_t                 dstQueueFamilyIndex)
{
    // Infer access masks if not explicitly provided
    if (srcAccessMask == vk::AccessFlagBits2{}) {
        srcAccessMask = inferAccessMaskFromStage(srcStageMask, true);
    }
    if (dstAccessMask == vk::AccessFlagBits2{}) {
        dstAccessMask = inferAccessMaskFromStage(dstStageMask, false);
    }

    auto bufferMemoryBarrier = vk::BufferMemoryBarrier2{
        srcStageMask,
        srcAccessMask,
        dstStageMask,
        dstAccessMask,
        srcQueueFamilyIndex,
        dstQueueFamilyIndex,
        buffer,
        offset,
        size};

    commandBuffer.pipelineBarrier2(
        vk::DependencyInfo{}.setBufferMemoryBarriers(bufferMemoryBarrier));
}

auto inferAccessMaskFromStage(
    vk::PipelineStageFlags2 stage,
    bool                    src) -> vk::AccessFlags2
{
    vk::AccessFlags2 access = {};

    // Handle each possible stage bit
    if ((stage | vk::PipelineStageFlagBits2::eComputeShader)
        == vk::PipelineStageFlagBits2::eComputeShader) {
        access |= (src) ? vk::AccessFlagBits2::eShaderRead
                        : vk::AccessFlagBits2::eShaderWrite;
    }

    if ((stage | vk::PipelineStageFlagBits2::eFragmentShader)
        == vk::PipelineStageFlagBits2::eFragmentShader) {
        access |= (src) ? vk::AccessFlagBits2::eShaderRead
                        : vk::AccessFlagBits2::eShaderWrite;
    }

    if ((stage | vk::PipelineStageFlagBits2::eVertexAttributeInput)
        == vk::PipelineStageFlagBits2::eVertexAttributeInput) {
        access |= vk::AccessFlagBits2::eVertexAttributeRead;
    }

    if ((stage | vk::PipelineStageFlagBits2::eTransfer)
        == vk::PipelineStageFlagBits2::eTransfer) {
        access |= (src) ? vk::AccessFlagBits2::eTransferRead
                        : vk::AccessFlagBits2::eTransferWrite;
    }

    assert(access != vk::AccessFlagBits2{});
    return access;
}

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format
{
    auto candidateFormats = std::array{

        // vk::Format::eD32Sfloat,
        // vk::Format::eD32SfloatS8Uint,
        // vk::Format::eD24UnormS8Uint,
        vk::Format::eD16Unorm,        // depth only
        vk::Format::eD32Sfloat,       // depth only
        vk::Format::eD32SfloatS8Uint, // depth-stencil
        vk::Format::eD24UnormS8Uint   // depth-stencil
    };

    for (auto format : candidateFormats) {
        auto properties = physicalDevice.getFormatProperties(format);

        if ((properties.optimalTilingFeatures
             & vk::FormatFeatureFlagBits::eDepthStencilAttachment)
            == vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            return format;
        }
    }

    assert(false);

    return vk::Format::eUndefined;
}
