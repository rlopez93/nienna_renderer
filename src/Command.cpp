#include "Command.hpp"
Command::Command(
    Device                    &device,
    Queue                     &queue,
    vk::CommandPoolCreateFlags poolFlags)
    :
      pool{
          device.handle,
          vk::CommandPoolCreateInfo{
              poolFlags,
              queue.index}},
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
auto Command::endSingleTime(
    Device &device,
    Queue  &queue) -> void
{
    // submit command buffer
    buffer.end();

    // create fence for synchronization
    auto fenceCreateInfo         = vk::FenceCreateInfo{};
    auto fence                   = vk::raii::Fence{device.handle, fenceCreateInfo};
    auto commandBufferSubmitInfo = vk::CommandBufferSubmitInfo{buffer};

    queue.handle.submit2(vk::SubmitInfo2{{}, {}, commandBufferSubmitInfo}, fence);
    auto result =
        device.handle.waitForFences(*fence, true, std::numeric_limits<uint64_t>::max());

    assert(result == vk::Result::eSuccess);
}
