#pragma once

struct Device;
struct ShaderInterface;
struct ResourceAllocator;
struct FrameContext;

struct ResourceBindings {
    static auto allocatePerFrame(
        Device                &device,
        const ShaderInterface &layout,
        ResourceAllocator     &allocator,
        FrameContext          &frames) -> void;
};
