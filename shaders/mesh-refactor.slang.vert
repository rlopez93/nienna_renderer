#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 12078 0
struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    vec4  data_0[4];
};


#line 22 1
struct Transform_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 modelMatrix_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 viewProjectionMatrix_0;
};



layout(binding = 0)
layout(std140) uniform block_Transform_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 modelMatrix_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 viewProjectionMatrix_0;
}transform_0;

#line 1
layout(location = 0)
out vec3 entryPointParam_vertexMain_normal_0;


#line 1
layout(location = 1)
out vec2 entryPointParam_vertexMain_uv_0;


#line 1
layout(location = 2)
out vec4 entryPointParam_vertexMain_color_0;


#line 1
layout(location = 0)
in vec3 input_position_0;


#line 1
layout(location = 1)
in vec3 input_normal_0;


#line 1
layout(location = 2)
in vec2 input_uv_0;


#line 1
layout(location = 3)
in vec4 input_color_0;


#line 10
struct FragmentPrimitive_0
{
    vec4 position_0;
    vec3 normal_0;
    vec2 uv_0;
    vec4 color_0;
};


#line 33
void main()
{
    FragmentPrimitive_0 output_0;
    output_0.position_0 = ((((((vec4(input_position_0, 1.0)) * (mat4x4(transform_0.modelMatrix_0.data_0[0][0], transform_0.modelMatrix_0.data_0[1][0], transform_0.modelMatrix_0.data_0[2][0], transform_0.modelMatrix_0.data_0[3][0], transform_0.modelMatrix_0.data_0[0][1], transform_0.modelMatrix_0.data_0[1][1], transform_0.modelMatrix_0.data_0[2][1], transform_0.modelMatrix_0.data_0[3][1], transform_0.modelMatrix_0.data_0[0][2], transform_0.modelMatrix_0.data_0[1][2], transform_0.modelMatrix_0.data_0[2][2], transform_0.modelMatrix_0.data_0[3][2], transform_0.modelMatrix_0.data_0[0][3], transform_0.modelMatrix_0.data_0[1][3], transform_0.modelMatrix_0.data_0[2][3], transform_0.modelMatrix_0.data_0[3][3]))))) * (mat4x4(transform_0.viewProjectionMatrix_0.data_0[0][0], transform_0.viewProjectionMatrix_0.data_0[1][0], transform_0.viewProjectionMatrix_0.data_0[2][0], transform_0.viewProjectionMatrix_0.data_0[3][0], transform_0.viewProjectionMatrix_0.data_0[0][1], transform_0.viewProjectionMatrix_0.data_0[1][1], transform_0.viewProjectionMatrix_0.data_0[2][1], transform_0.viewProjectionMatrix_0.data_0[3][1], transform_0.viewProjectionMatrix_0.data_0[0][2], transform_0.viewProjectionMatrix_0.data_0[1][2], transform_0.viewProjectionMatrix_0.data_0[2][2], transform_0.viewProjectionMatrix_0.data_0[3][2], transform_0.viewProjectionMatrix_0.data_0[0][3], transform_0.viewProjectionMatrix_0.data_0[1][3], transform_0.viewProjectionMatrix_0.data_0[2][3], transform_0.viewProjectionMatrix_0.data_0[3][3]))));

#line 44
    output_0.normal_0 = input_normal_0;
    output_0.uv_0 = input_uv_0;
    output_0.color_0 = input_color_0;
    FragmentPrimitive_0 _S1 = output_0;

#line 47
    gl_Position = output_0.position_0;

#line 47
    entryPointParam_vertexMain_normal_0 = _S1.normal_0;

#line 47
    entryPointParam_vertexMain_uv_0 = _S1.uv_0;

#line 47
    entryPointParam_vertexMain_color_0 = _S1.color_0;

#line 47
    return;
}

