#pragma once

#include "vulkan_raii.hpp"

#ifdef NDEBUG
#define VK_CHECK(vkFnc) vkFnc
#else
#include <cassert>
#include <fmt/base.h>
#include <vulkan/vk_enum_string_helper.h>
#define VK_CHECK(vkFnc)                                                                \
    {                                                                                  \
        if (const VkResult checkResult = (vkFnc); checkResult != VK_SUCCESS) {         \
            const char *errMsg = string_VkResult(checkResult);                         \
            fmt::print(stderr, "Vulkan error: {}", errMsg);                            \
            assert(checkResult == VK_SUCCESS);                                         \
        }                                                                              \
    }
#endif

auto inferAccessMaskFromStage(
    vk::PipelineStageFlags2 stage,
    bool                    src) -> vk::AccessFlags2;

void cmdBufferMemoryBarrier(
    vk::raii::CommandBuffer &commandBuffer,
    vk::Buffer              &buffer,
    vk::PipelineStageFlags2  srcStageMask,
    vk::PipelineStageFlags2  dstStageMask,
    vk::AccessFlags2         srcAccessMask = {}, // Default to infer if not provided
    vk::AccessFlags2         dstAccessMask = {}, // Default to infer if not provided
    vk::DeviceSize           offset        = 0,
    vk::DeviceSize           size          = vk::WholeSize,
    uint32_t                 srcQueueFamilyIndex = vk::QueueFamilyIgnored,
    uint32_t                 dstQueueFamilyIndex = vk::QueueFamilyIgnored);

auto makePipelineStageAccessTuple(vk::ImageLayout state) -> std::tuple<
    vk::PipelineStageFlags2,
    vk::AccessFlags2>;

void cmdTransitionImageLayout(
    vk::raii::CommandBuffer &cmd,
    VkImage                  image,
    vk::ImageLayout          oldLayout,
    vk::ImageLayout          newLayout,
    vk::ImageAspectFlags     aspectMask = vk::ImageAspectFlagBits::eColor)

    ;

auto beginSingleTimeCommands(
    vk::raii::Device      &device,
    vk::raii::CommandPool &commandPool) -> vk::raii::CommandBuffer;

void endSingleTimeCommands(
    vk::raii::Device        &device,
    vk::raii::CommandPool   &commandPool,
    vk::raii::CommandBuffer &commandBuffer,
    vk::raii::Queue         &queue);

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format;
