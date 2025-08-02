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
