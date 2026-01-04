#include "ImageLayoutState.hpp"

auto imageUseInfo(ImageUse use) -> ImageUseInfo
{
    switch (use) {
    case ImageUse::kColorAttachmentWrite:
        return ImageUseInfo{
            .layout = vk::ImageLayout::eColorAttachmentOptimal,
            .stage  = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
            .access = vk::AccessFlagBits2::eColorAttachmentWrite
                    | vk::AccessFlagBits2::eColorAttachmentRead,
        };

    case ImageUse::kDepthAttachmentWrite:
        return ImageUseInfo{
            .layout = vk::ImageLayout::eDepthAttachmentOptimal,
            .stage  = vk::PipelineStageFlagBits2::eEarlyFragmentTests
                   | vk::PipelineStageFlagBits2::eLateFragmentTests,
            .access = vk::AccessFlagBits2::eDepthStencilAttachmentWrite
                    | vk::AccessFlagBits2::eDepthStencilAttachmentRead,
        };

    case ImageUse::kShaderSampledRead:
        return ImageUseInfo{
            .layout = vk::ImageLayout::eShaderReadOnlyOptimal,
            .stage  = vk::PipelineStageFlagBits2::eFragmentShader
                   | vk::PipelineStageFlagBits2::eComputeShader,
            .access = vk::AccessFlagBits2::eShaderSampledRead,
        };

    case ImageUse::kTransferDstWrite:
        return ImageUseInfo{
            .layout = vk::ImageLayout::eTransferDstOptimal,
            .stage  = vk::PipelineStageFlagBits2::eTransfer,
            .access = vk::AccessFlagBits2::eTransferWrite,
        };

    case ImageUse::kPresent:
        return ImageUseInfo{
            .layout = vk::ImageLayout::ePresentSrcKHR,
            .stage  = vk::PipelineStageFlagBits2::eNone,
            .access = vk::AccessFlagBits2::eNone,
        };
    }

    return {};
}

auto ImageLayoutState::reset() -> void
{
    entries.clear();
}

auto ImageLayoutState::forgetImages(std::span<const vk::Image> imgs) -> void
{
    for (vk::Image img : imgs) {
        entries.erase(static_cast<VkImage>(img));
    }
}

void ImageLayoutState::transition(
    vk::raii::CommandBuffer  &cmd,
    vk::Image                 image,
    vk::ImageSubresourceRange range,
    ImageUse                  newUse)
{
    const auto key = static_cast<VkImage>(image);

    Entry &e = entries[key];

    if (!e.firstUse && e.use == newUse) {
        return;
    }

    const auto dst = imageUseInfo(newUse);

    const auto oldLayout = e.firstUse ? vk::ImageLayout::eUndefined : e.info.layout;

    const auto srcStage = e.firstUse ? vk::PipelineStageFlagBits2::eNone : e.info.stage;

    const auto srcAccess = e.firstUse ? vk::AccessFlagBits2::eNone : e.info.access;

    const auto barrier = vk::ImageMemoryBarrier2{
        srcStage,
        srcAccess,
        dst.stage,
        dst.access,
        oldLayout,
        dst.layout,
        vk::QueueFamilyIgnored,
        vk::QueueFamilyIgnored,
        image,
        range,
    };

    cmd.pipelineBarrier2(vk::DependencyInfo{}.setImageMemoryBarriers(barrier));

    e.firstUse = false;
    e.use      = newUse;
    e.info     = dst;
}
