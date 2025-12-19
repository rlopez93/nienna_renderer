#include "RenderContext.hpp"

RenderContext::RenderContext(
    Window                          &window,
    const std::vector<const char *> &requiredExtensions)
    : instance{},
      surface{
          instance,
          window},
      physicalDevice{
          instance,
          requiredExtensions},
      device{
          physicalDevice,
          window,
          surface,
          requiredExtensions}
{
}
