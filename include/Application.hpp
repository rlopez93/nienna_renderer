#pragma once
#include "RenderContext.hpp"
#include "Renderer.hpp"
#include "Window.hpp"

struct Application {
    Window        window;
    RenderContext renderContext;
    Renderer      renderer;

    void run();
};
