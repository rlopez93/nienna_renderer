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

auto beginSingleTimeCommands(
    vk::raii::Device      &device,
    vk::raii::CommandPool &commandPool) -> vk::raii::CommandBuffer
{
    auto commandBuffers = vk::raii::CommandBuffers{
        device,
        vk::CommandBufferAllocateInfo{
            commandPool,
            vk::CommandBufferLevel::ePrimary,
            1}};
    commandBuffers.front().begin(
        vk::CommandBufferBeginInfo{vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    return std::move(commandBuffers.front());
};
void endSingleTimeCommands(
    vk::raii::Device        &device,
    vk::raii::CommandPool   &commandPool,
    vk::raii::CommandBuffer &commandBuffer,
    vk::raii::Queue         &queue)
{
    // submit command buffer
    commandBuffer.end();

    // create fence for synchronization
    auto fenceCreateInfo         = vk::FenceCreateInfo{};
    auto fence                   = vk::raii::Fence{device, fenceCreateInfo};
    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{commandBuffer};

    queue.submit2(vk::SubmitInfo2{{}, {}, commandBufferSubmitInfo}, fence);
    auto result =
        device.waitForFences(*fence, true, std::numeric_limits<uint64_t>::max());

    assert(result == vk::Result::eSuccess);
}
