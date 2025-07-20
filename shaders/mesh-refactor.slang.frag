#version 450
#extension GL_EXT_nonuniform_qualifier : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 51 0
layout(binding = 1)
uniform sampler2D  texture_0[];


#line 51
struct EntryPointParams_std430_0
{
    uint textureIndex_0;
};


#line 16
layout(push_constant)
layout(std430) uniform block_EntryPointParams_std430_0
{
    uint textureIndex_0;
}entryPointParams_0;

#line 970 1
layout(location = 0)
out vec4 entryPointParam_fragmentMain_color_0;


#line 970
layout(location = 1)
in vec2 input_uv_0;


#line 970
layout(location = 2)
in vec4 input_color_0;


#line 25 0
struct FragmentOutput_0
{
    vec4 color_0;
};


#line 54
void main()
{
    FragmentOutput_0 output_0;

#line 61
    output_0.color_0 = (texture((texture_0[entryPointParams_0.textureIndex_0]), (input_uv_0))) * input_color_0;

#line 61
    entryPointParam_fragmentMain_color_0 = output_0.color_0;

#line 61
    return;
}

