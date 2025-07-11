#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 39 0
layout(binding = 1)
uniform sampler2D texture_0;


#line 962 1
layout(location = 0)
out vec4 entryPointParam_fragmentMain_color_0;


#line 962
layout(location = 1)
in vec2 input_uv_0;


#line 22 0
struct FragmentOutput_0
{
    vec4 color_0;
};


#line 42
void main()
{
    FragmentOutput_0 output_0;
    output_0.color_0 = (texture((texture_0), (input_uv_0)));

#line 45
    entryPointParam_fragmentMain_color_0 = output_0.color_0;

#line 45
    return;
}

