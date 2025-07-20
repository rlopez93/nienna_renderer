#version 450
#extension GL_EXT_nonuniform_qualifier : require
layout(row_major) uniform;
layout(row_major) buffer;

#line 53 0
struct Index_std430_0
{
    uint index_0;
};


#line 56
layout(push_constant)
layout(std430) uniform block_Index_std430_0
{
    layout(offset = 4) uint index_0;
}textureIndex_0;

#line 50
layout(binding = 1)
uniform sampler2D  texture_0[];


#line 11969 1
layout(location = 0)
out vec4 entryPointParam_fragmentMain_color_0;


#line 11969
layout(location = 1)
in vec2 input_uv_0;


#line 11969
layout(location = 2)
in vec4 input_color_0;


#line 25 0
struct FragmentOutput_0
{
    vec4 color_0;
};


#line 59
void main()
{
    FragmentOutput_0 output_0;

#line 66
    output_0.color_0 = vec4(dot((texture((texture_0[textureIndex_0.index_0]), (input_uv_0))), input_color_0));

#line 66
    entryPointParam_fragmentMain_color_0 = output_0.color_0;

#line 66
    return;
}

