# Nienna

WIP C++26 real-time 3D renderer and glTF 2.0 viewer targeting Vulkan 1.4

## Current features

* full Vulkan dynamic rendering pipeline
* basic glTF 2.0 scene loading
  * mesh primitives
  * baseColorFactor/baseColorTexture
  * cameras
* camera movement (currently buggy)
* Lambert diffuse directional light
* simple debug views (normals, wireframe)
* puts triangles on screen
  * many of them, in fact

## Planned features

* support for multi-scene glTF files
* support for multi-primitive meshes
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
* VulkanSDK 1.4.335
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
* HEAVY use of Vulkan Configurator validation layers
* nvpro-samples/vk_minimal_latest
* and just Sascha Willems, in general

## Build

```bash
cmake -B build -S .
cmake --build build
