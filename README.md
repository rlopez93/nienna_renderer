# Nienna

WIP C++26 real-time 3D renderer and glTF 2.0 viewer targeting Vulkan 1.4

## Current features

* full Vulkan dynamic rendering pipeline
  * frames in flight synchronized with a timeline semaphore
  * swapchain recreation on window resize
* basic glTF 2.0 scene loading
  * node hierarchies with TRS transforms
  * meshes with multiple primitives
  * baseColorFactor/baseColorTexture, vertex colors, two UV sets
  * texture samplers (wrap and filter modes)
  * embedded and external images
  * perspective and orthographic cameras, plus a default camera that frames the whole scene
* Lambert diffuse directional light
* simple debug views (normals, wireframe), cycled with `]`
* puts triangles on screen
  * many of them, in fact

## Planned features

* support for multi-scene glTF files
* camera movement
* core glTF PBR support + KHR extensions
* ImGui integration for on-screen debug UI

## Rendered with Nienna

Assets from KhronosGroup/glTF-Sample-Assets:
<img width="800" height="600" alt="Screenshot_20260101_225749" src="https://github.com/user-attachments/assets/0e41b681-5c1a-4bb2-9c79-e725db569e1b" />
<img width="800" height="600" alt="Screenshot_20260101_225753" src="https://github.com/user-attachments/assets/89a1ab9e-9f4d-4e9b-846e-0739a2c739c3" />
<img width="800" height="600" alt="Screenshot_20260101_225755" src="https://github.com/user-attachments/assets/45b83710-2193-49a0-9844-fcd5dca7af97" />
<img width="800" height="600" alt="Screenshot_20251215_080705" src="https://github.com/user-attachments/assets/c2d69337-369f-4f4c-ac1d-8b3e4c632654" />
<img width="800" height="600" alt="Screenshot_20260104_151048" src="https://github.com/user-attachments/assets/a6ca6f70-43f1-4e1d-8d6d-1fea45ee5ec0" />

## Tools & Libraries

* C++26
* Vulkan 1.4
* VulkanSDK 1.4.363
* Vulkan-Hpp RAII bindings
* shaders written in Slang
* SDL3 windowing and event handling
* glTF 2.0 loading with fastgltf
* VulkanMemoryAllocator
* texture loading with stb_image
* glm for math, fmt for formatting

Tested on Manjaro KDE Linux 26.1.3 with:

* GCC 16.2.1 with C++26
* CMake 4.4.4
* VulkanSDK 1.4.363.1
* SDL3 3.4.18-1
* fmt 12.2.0-1
* glm 1.0.3-1

## Acknowledgements

This project leans on (and borrows heavily from) the following resources:

* the Khronos Vulkan Tutorial
* the Vulkan documentation at docs.vulkan.org
* KhronosGroup repositories, in particular Vulkan-Samples, glTF-Sample-Assets, Vulkan-Hpp and Vulkan-Docs
* the LunarG Vulkan SDK
* the Vulkan validation layers, configured through Vulkan Configurator
* nvpro-samples/vk_minimal_latest
* Sascha Willems' Vulkan examples

## Build

Requirements:

* the Vulkan SDK (1.4.x), set up so `VULKAN_SDK` is set and `slangc` is on your `PATH` (e.g. by sourcing the SDK's `setup-env.sh`)
* SDL3, fmt and glm (system packages)
* CMake 3.25 or newer, a C++26 compiler, and Ninja

```bash
git submodule update --init --recursive
cmake --preset debug
cmake --build --preset debug
```

Debug builds go in `build/debug/`. A `release` preset is also available; it builds into `build/release/`.

## Run

From the repository root:

```bash
./build/debug/nienna [path/to/model.gltf] [path/to/shader.spv]
```

With no arguments it loads the Duck from glTF-Sample-Assets and the default shader from `build/debug/_autogen/`. For a release build, pass the shader path, e.g. `build/release/_autogen/mesh_basepass.slang.spv`. Press `]` to cycle between the shaded, normals and wireframe views.
