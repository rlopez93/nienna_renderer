#include "ResourceBindings.hpp"
#include "FrameContext.hpp"
#include "ResourceAllocator.hpp"
#include "ResourceLayout.hpp"

auto ResourceBindings::allocatePerFrame(
    vk::raii::Device     &device,
    const ResourceLayout &layout,
    ResourceAllocator    &allocator,
    FrameContext         &frames) -> void
{
    frames.resourceBindings.clear();
    frames.resourceBindings.reserve(frames.maxFramesInFlight);

    auto layouts =
        std::vector<vk::DescriptorSetLayout>(frames.maxFramesInFlight, *layout.handle);

    auto allocInfo = vk::DescriptorSetAllocateInfo{*allocator.handle, layouts};

    auto sets = device.allocateDescriptorSets(allocInfo);

    for (auto &set : sets) {
        frames.resourceBindings.emplace_back(std::move(set));
    }
}
