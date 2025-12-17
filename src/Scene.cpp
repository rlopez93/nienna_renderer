#include "Scene.hpp"

auto Scene::processInput(SDL_Event &e) -> void
{
    if (e.type == SDL_EVENT_KEY_DOWN && !e.key.repeat) {
        switch (e.key.scancode) {
        case SDL_SCANCODE_N:
            cameraIndex = (cameraIndex + 1) % cameras.size();
        default:
            break;
        }
    }

    getCamera().processInput(e);
    // pointLight.processInput(e);
}

// void PointLight::processInput(const SDL_Event &e)
// {
//     if (e.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
//         switch (e.gbutton.button) {
//         case SDL_GAMEPAD_BUTTON_SOUTH:
//             fmt::println("ha ha i pressed x");
//             break;
//
//         case SDL_GAMEPAD_BUTTON_WEST:
//             fmt::println("he he i pressed square");
//         }
//     }
//
//     else if (e.type == SDL_EVENT_GAMEPAD_BUTTON_UP) {
//         switch (e.gbutton.button) {
//         case SDL_GAMEPAD_BUTTON_SOUTH:
//             fmt::println("uh oh i let go of x :(");
//             break;
//         case SDL_GAMEPAD_BUTTON_WEST:
//             fmt::println("aw heck i let go of square :(");
//             break;
//         }
//     }
// }
//
auto Scene::getCamera() const -> const PerspectiveCamera &
{
    return cameras[cameraIndex];
}

auto Scene::getCamera() -> PerspectiveCamera &
{
    return cameras[cameraIndex];
}

auto Scene::update(const std::chrono::duration<float> dt) -> void
{
    getCamera().update(dt);
}
