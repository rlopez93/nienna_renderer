#include "LayoutTracker.hpp"

#include <cassert>

LayoutTracker::LayoutTracker(std::uint32_t swapImgCount)
{
    onSwapchainRecreated(swapImgCount);
}

void LayoutTracker::onSwapchainRecreated(std::uint32_t swapImgCount)
{
    swapchainLayouts =
        std::vector<vk::ImageLayout>(swapImgCount, vk::ImageLayout::eUndefined);
}

void LayoutTracker::registerNamedImage(
    ImageName       name,
    vk::ImageLayout initialLayout)
{
    const auto idx = static_cast<std::size_t>(name);
    assert(idx < kNameCount);
    assert(!namedRegistered.test(idx));

    namedRegistered.set(idx);
    namedLayouts[idx] = initialLayout;
}

void LayoutTracker::resetNamedImage(
    ImageName       name,
    vk::ImageLayout initialLayout)
{
    const auto idx = static_cast<std::size_t>(name);
    assert(idx < kNameCount);
    assert(namedRegistered.test(idx));

    namedLayouts[idx] = initialLayout;
}

auto LayoutTracker::swapchainLayout(std::uint32_t imageIndex) const -> vk::ImageLayout
{
    assert(imageIndex < swapchainLayouts.size());
    return swapchainLayouts[imageIndex];
}

auto LayoutTracker::namedLayout(ImageName name) const -> vk::ImageLayout
{
    const auto idx = static_cast<std::size_t>(name);
    assert(idx < kNameCount);
    assert(namedRegistered.test(idx));

    return namedLayouts[idx];
}

void LayoutTracker::setSwapchainLayout(
    std::uint32_t   imageIndex,
    vk::ImageLayout layout)
{
    assert(imageIndex < swapchainLayouts.size());
    swapchainLayouts[imageIndex] = layout;
}

void LayoutTracker::setNamedLayout(
    ImageName       name,
    vk::ImageLayout layout)
{
    const auto idx = static_cast<std::size_t>(name);
    assert(idx < kNameCount);
    assert(namedRegistered.test(idx));

    namedLayouts[idx] = layout;
}

void LayoutTracker::expectSwapchainLayout(
    std::uint32_t   imageIndex,
    vk::ImageLayout expected) const
{
    assert(imageIndex < swapchainLayouts.size());
    assert(swapchainLayouts[imageIndex] == expected);
}

void LayoutTracker::expectNamedLayout(
    ImageName       name,
    vk::ImageLayout expected) const
{
    const auto idx = static_cast<std::size_t>(name);
    assert(idx < kNameCount);
    assert(namedRegistered.test(idx));
    assert(namedLayouts[idx] == expected);
}

void LayoutTracker::transitionSwapchain(
    std::uint32_t   imageIndex,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout)
{
    expectSwapchainLayout(imageIndex, oldLayout);
    setSwapchainLayout(imageIndex, newLayout);
}

void LayoutTracker::transitionNamed(
    ImageName       name,
    vk::ImageLayout oldLayout,
    vk::ImageLayout newLayout)
{
    expectNamedLayout(name, oldLayout);
    setNamedLayout(name, newLayout);
}
