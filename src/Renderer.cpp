#include "Renderer.hpp"

auto Renderer::present() -> void
{
    auto renderFinishedSemaphore = ctx.swapchain.getRenderFinishedSemaphore();
    auto presentResult           = ctx.present.handle.presentKHR(
        vk::PresentInfoKHR{
            renderFinishedSemaphore,
            *ctx.swapchain.swapchain,
            ctx.swapchain.nextImageIndex});

    if (presentResult == vk::Result::eErrorOutOfDateKHR
        || presentResult == vk::Result::eSuboptimalKHR) {
        ctx.swapchain.needRecreate = true;
    }

    else if (!(presentResult == vk::Result::eSuccess
               || presentResult == vk::Result::eSuboptimalKHR)) {
        throw std::exception{};
    }

    // advance to the next frame in the swapchain
    ctx.swapchain.advanceFrame();
}
auto RenderContext::getWindowExtent() const -> vk::Extent2D
{
    return physicalDevice.handle.getSurfaceCapabilitiesKHR(surface.handle)
        .currentExtent;
}
