#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 1 0
layout(location = 0)
out vec3 entryPointParam_vertexMain_uv_0;


#line 1
layout(location = 0)
in vec3 input_position_0;


#line 1
layout(location = 2)
in vec3 input_uv_0;


#line 8
struct VertexOutput_0
{
    vec4 position_0;
    vec3 uv_0;
};


#line 20
void main()
{
    VertexOutput_0 output_0;
    output_0.position_0 = vec4(input_position_0, 1.0);
    output_0.uv_0 = input_uv_0;
    VertexOutput_0 _S1 = output_0;

#line 25
    gl_Position = output_0.position_0;

#line 25
    entryPointParam_vertexMain_uv_0 = _S1.uv_0;

#line 25
    return;
}

