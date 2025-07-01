#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 8 0
layout(location = 0)
out vec4 entryPointParam_fragmentMain_color_0;




struct FragmentOutput_0
{
    vec4 color_0;
};


#line 29
void main()
{
    FragmentOutput_0 output_0;
    output_0.color_0 = vec4(0.5, 0.20000000298023224, 0.10000000149011612, 1.0);

#line 32
    entryPointParam_fragmentMain_color_0 = output_0.color_0;

#line 32
    return;
}

