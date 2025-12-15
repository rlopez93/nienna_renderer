# Nienna

WIP real-time 3D graphics renderer targeting Vulkan 1.4

## Current features

* full Vulkan dynamic rendering pipeline
* basic glTF 2.0 scene loading
  * mesh primitives
  * baseColorFactor/baseColorTexture
  * cameras
* camera movement (currently buggy)
* Lambert diffuse lighting
* timeline semaphores
* puts triangle on screen
  * many of them, in fact

## Planned features

* basic lighting and shading improvements
* simple material support
* ImGui integration for on-screen debug UI

## Rendered with Nienna

## Tools & Libraries

* Vulkan 1.4
* C++26
* Vulkan-Hpp RAII bindings
* shaders written in Slang
* SDL3 windowing and event handling
* glTF 2.0 loading with fastgltf
* VulkanMemoryAllocator
* texture loading with stb_image

Tested on Manjaro KDE Linux 25.0 with:

* GCC 15.2.1 with C++26
* CMake 4.2.1
* VulkanSDK 1.4.335.0
* SDL3 3.2.28-1
* fmt 12.1.0-1
* glm 1.0.2-1

## Acknowledgements

Wouldn't have made it this far without:

* Khronos Vulkan tutorial (I did it BEFORE it used dynamic rendering *hmm*)
* docs.vulkan.org
* so many KhronosGroup repos, but in particular Vulkan-Samples, glTF-Sample-Assets, Vulkan-Hpp (of course) and Vulkan-Docs
* the LunarG VulkanSDK
* nvpro-samples/vk_minimal_latest
* and just Sascha Willems, in general

## Build

```bash
cmake -B build -S .
cmake --build build
