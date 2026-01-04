#include "Utility.hpp"

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
