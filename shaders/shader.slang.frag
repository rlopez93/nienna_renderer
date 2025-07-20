#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 42 0
layout(binding = 1)
uniform sampler2D texture_0;


#line 970 1
layout(location = 0)
out vec4 entryPointParam_fragmentMain_color_0;


#line 2194 2
layout(location = 0)
in vec3 input_normal_0;


#line 2194
layout(location = 1)
in vec2 input_uv_0;


#line 22 0
struct FragmentOutput_0
{
    vec4 color_0;
};


#line 45
void main()
{
    FragmentOutput_0 output_0;
    vec2 _S1 = input_normal_0.xy;

#line 48
    output_0.color_0 = (texture((texture_0), (_S1 + input_uv_0 - _S1)));

#line 48
    entryPointParam_fragmentMain_color_0 = output_0.color_0;

#line 48
    return;
}

