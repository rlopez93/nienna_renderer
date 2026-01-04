#pragma once

#include <vulkan/vulkan_raii.hpp>

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

auto findDepthFormat(vk::raii::PhysicalDevice &physicalDevice) -> vk::Format;
