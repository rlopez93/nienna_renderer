#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <vector>

#include <vulkan/vulkan.hpp>

enum class ImageName : std::uint32_t {
    MainDepth = 0,
    SceneColorLdr,
    // Future: HdrColor, MotionVectors, GBuffer*, etc.
    Count
};

class LayoutTracker
{
  public:
    LayoutTracker() = default;

    explicit LayoutTracker(std::uint32_t swapImgCount);

    // Call after swapchain (re)create. Sets all swap layouts to
    // eUndefined (first use rule).
    void onSwapchainRecreated(std::uint32_t swapImgCount);

    // Named images are renderer-owned (non-swapchain) targets.
    void registerNamedImage(
        ImageName       name,
        vk::ImageLayout initialLayout);

    void resetNamedImage(
        ImageName       name,
        vk::ImageLayout initialLayout);

    [[nodiscard]]
    auto swapchainLayout(std::uint32_t imageIndex) const -> vk::ImageLayout;

    [[nodiscard]]
    auto namedLayout(ImageName name) const -> vk::ImageLayout;

    void setSwapchainLayout(
        std::uint32_t   imageIndex,
        vk::ImageLayout layout);

    void setNamedLayout(
        ImageName       name,
        vk::ImageLayout layout);

    // Debug assertions: verify a resource is in the expected layout
    // before starting a pass.
    void expectSwapchainLayout(
        std::uint32_t   imageIndex,
        vk::ImageLayout expected) const;

    void expectNamedLayout(
        ImageName       name,
        vk::ImageLayout expected) const;

    // Convenience: asserts current == oldLayout, then sets newLayout.
    void transitionSwapchain(
        std::uint32_t   imageIndex,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout);

    void transitionNamed(
        ImageName       name,
        vk::ImageLayout oldLayout,
        vk::ImageLayout newLayout);

  private:
    static constexpr std::size_t kNameCount =
        static_cast<std::size_t>(ImageName::Count);

    std::vector<vk::ImageLayout> swapchainLayouts;

    std::array<vk::ImageLayout, kNameCount> namedLayouts{};
    std::bitset<kNameCount>                 namedRegistered{};
};
