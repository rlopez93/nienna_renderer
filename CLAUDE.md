# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

WIP Vulkan 1.4 glTF viewer in C++26. See README.md for the feature list, dependencies and tested versions.

## Build and run

The Vulkan SDK must be sourced first (`VULKAN_SDK` set, `slangc` on `PATH`).

```bash
cmake --preset debug && cmake --build --preset debug   # or the release preset
./build/debug/nienna [model.gltf] [shader.spv]         # run from the repo root (default paths are relative)
```

- There are no tests. There is no lint target either, but `.clang-tidy` and `.clang-format` are configured, and `compile_commands.json` is written to the build dir. For example, run `clang-tidy -p build/debug src/Foo.cpp`.
- Source files are listed explicitly in `CMakeLists.txt`, so new `.cpp` files must be added there. Shaders are globbed: every `shaders/*.slang` is compiled to `build/<preset>/_autogen/<name>.slang.spv`, so a broken experimental shader breaks the build.

## Code style

- Format with clang-format, using the repo's `.clang-format`. It aligns consecutive assignments and declarations, and breaks after open brackets.
- Use trailing return types (`auto f() -> T`) and `auto` for locals. This is enforced by `.clang-tidy`'s `modernize-use-trailing-return-type` and `modernize-use-auto` checks.
- Use designated initializers for config and data structs.
- Prefer Vulkan-Hpp RAII types (`vk::raii::*`). Wrap raw VMA calls with `VK_CHECK` (see `Utility.hpp`).

## Architecture

Ownership flows top-down from `src/main.cpp`, which currently wires everything together. `Application.hpp` is an unused stub.

1. **`RenderContext`** owns the Vulkan core objects in order: `Instance`, `Surface`, `PhysicalDevice`, `Device` (queues), `Allocator` (VMA), `Swapchain` and `RenderTargets` (depth, etc.). `main.cpp` passes in the required device extensions.
2. **Asset loading** has two layers:
   - `GltfLoader` (`getAsset`) converts fastgltf data into a CPU-side **`RenderAsset`**: nodes with TRS, meshes and primitives, materials, textures and cameras.
   - `buildSceneView` flattens the active scene's node hierarchy into a **`SceneView`**: `NodeInstance`s with world matrices, `DrawItem`s and `CameraInstance`s. `main.cpp` then appends a default camera framed on the scene AABB, which is always the last entry in `asset.cameras`.
3. **`RenderableResources`** uploads a `RenderAsset` and `SceneView` to the GPU. It creates vertex and index buffers, texture images with both sRGB and linear views, deduplicated samplers (via `samplerCache`), and the materials SSBO (see `MaterialPacking`). It also writes each frame's descriptor set.
4. **`Renderer`** owns the pipeline, descriptor pool and layout (`ShaderInterface`), plus a **`FrameContext`**. `FrameContext` holds frames-in-flight state: per-frame command buffers, a frame UBO and node-instance SSBO, and descriptor sets, synchronized by a single timeline semaphore. Each frame runs `beginFrame()` (which recreates the swapchain if flagged, then returns false to skip the frame when acquire fails, e.g. out of date), then the app updates the UBO, SSBO and descriptors, then `render()` and `endFrame()` (submit and present). It uses dynamic rendering, with no render passes. `ImageLayoutState` tracks image layouts for barriers.

### CPU/GPU shared interface

`include/ShaderInterfaceTypes.hpp` is compiled as both C++ and Slang (`#include`d by `shaders/mesh_basepass.slang`). It defines the descriptor binding indices (`kBinding*`), and the UBO, SSBO and push-constant structs, with `static_assert`s on size and alignment. Shaders are compiled with `-force-glsl-scalar-layout`.

When changing these structs or bindings, keep the following in sync:
- the struct definitions and their `static_assert`s
- the `ShaderInterfaceDescription` built in `main.cpp`, which sets descriptor types, counts and stages. Texture arrays are sized by `asset.textures.size()`.
- `RenderableResources::updateDescriptorSet`
- the vertex input layout in `Pipeline.cpp`, which must match the `VSInput` locations in the shader (position, normal, tangent, uv0, uv1, color)

### Stale files

`include/Application.hpp`, `shaders/shader.vert` and `shader.frag`, and the other `.slang` files besides `mesh_basepass.slang` are unused leftovers or experiments, kept intentionally. Don't delete or edit them unless asked.

## Known issues

- `ImageLayoutState::transition` returns early (no barrier) when an image's use is unchanged. The single shared `mainDepth` therefore gets no barrier between frames in flight: a write-after-write hazard between one frame's depth writes and the next frame's clear.
- The swapchain image's transition to color attachment uses `srcStage = eNone` (first use and from `kPresent`), so it doesn't chain with the acquire semaphore wait (`eAllCommands`), and the layout transition isn't ordered after the acquire. Fix: wait at `eColorAttachmentOutput` and use that as the barrier's source stage.
- The shader's missing-normal fallback (`dot(N, N) < 1e-5`) runs after `normalize()`, which gives NaN for a zero normal, so it never fires; models without `NORMAL` get NaN lighting.
- `Swapchain::createSwapchain` throws on a zero extent, and nothing catches it. Minimizing (on Windows) or resizing to zero height crashes when the swapchain is recreated.

- `Allocator::freeStagingBuffers()` is never called after upload, so staging buffers live for the whole run. It is safe to call after `RenderableResources::create()`, which waits on a fence.
- `Renderer::beginFrame` treats `eSuboptimalKHR` from acquire as failure and returns without waiting on the acquire semaphore, leaving it pending; the next acquire reuses it.
- The pipeline bakes in back-face culling and no blending; materials' `cullMode`, `alphaBlendEnable` and `alphaMaskEnable` are loaded but never applied, and the shader doesn't discard below alpha cutoff.
- The shader transforms normals by the plain model matrix (wrong under non-uniform scale).
- `findDepthFormat` prefers D16.
- Descriptor sets are rewritten every frame.
- Textures are uploaded without mipmaps.
- `sceneColorLdr` is allocated and recreated but never rendered to; `reflectShader`'s result is discarded; `processEvents` and `chooseSurfaceFormat` in `main.cpp` are unused.
- Device creation requires unused extensions (e.g. `VK_EXT_host_image_copy`) and enables every supported feature.
- The glTF parser declares IOR, specular and clearcoat but the loader never reads them; `KHR_texture_transform` isn't parsed.
- On Windows the debug callback shows a message box instead of printing.
