#include "Command.hpp"
Command::Command(
    Device                    &device,
    vk::CommandPoolCreateFlags poolFlags)
    :
      pool{
          device.handle,
          vk::CommandPoolCreateInfo{
              poolFlags,
              device.queueFamilyIndices.graphicsIndex}},
      buffer{std::move(
          vk::raii::CommandBuffers{
              device.handle,
              vk::CommandBufferAllocateInfo{
                  pool,
                  vk::CommandBufferLevel::ePrimary,
                  1}}
              .front())}
{
}
auto Command::beginSingleTime() -> void
{
    buffer.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
}
auto Command::endSingleTime(Device &device) -> void
{
    // submit command buffer
    buffer.end();

    // create fence for synchronization
    auto fenceCreateInfo         = vk::FenceCreateInfo{};
    auto fence                   = vk::raii::Fence{device.handle, fenceCreateInfo};
    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{buffer};

    device.graphicsQueue.submit2(
        vk::SubmitInfo2{{}, {}, commandBufferSubmitInfo},
        fence);
    auto result =
        device.handle.waitForFences(*fence, true, std::numeric_limits<uint64_t>::max());

    assert(result == vk::Result::eSuccess);
}
