#version 450
layout(row_major) uniform;
layout(row_major) buffer;

#line 11760 0
struct _MatrixStorage_float4x4_ColMajorstd140_0
{
    vec4 data_0[4];
};

#line 18 1
struct Scene_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 model_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 view_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 projection_0;
};

layout(binding = 0)
layout(std140) uniform block_Scene_std140_0
{
    _MatrixStorage_float4x4_ColMajorstd140_0 model_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 view_0;
    _MatrixStorage_float4x4_ColMajorstd140_0 projection_0;
} scene_0;

#line 1
layout(location = 0)
out vec3 entryPointParam_vertexMain_normal_0;

#line 1
layout(location = 1)
out vec2 entryPointParam_vertexMain_uv_0;

#line 1
layout(location = 0)
in vec3 input_position_0;

#line 1
layout(location = 1)
in vec3 input_normal_0;

#line 1
layout(location = 2)
in vec2 input_uv_0;

#line 8
struct VertexOutput_0
{
    vec4 position_0;
    vec3 normal_0;
    vec2 uv_0;
};

#line 30
void main()
{
    VertexOutput_0 output_0;
    output_0.position_0 = (((((((((vec4(input_position_0, 1.0)) * (mat4x4(scene_0.model_0.data_0[0][0], scene_0.model_0.data_0[1][0], scene_0.model_0.data_0[2][0], scene_0.model_0.data_0[3][0], scene_0.model_0.data_0[0][1], scene_0.model_0.data_0[1][1], scene_0.model_0.data_0[2][1], scene_0.model_0.data_0[3][1], scene_0.model_0.data_0[0][2], scene_0.model_0.data_0[1][2], scene_0.model_0.data_0[2][2], scene_0.model_0.data_0[3][2], scene_0.model_0.data_0[0][3], scene_0.model_0.data_0[1][3], scene_0.model_0.data_0[2][3], scene_0.model_0.data_0[3][3]))))) * (mat4x4(scene_0.view_0.data_0[0][0], scene_0.view_0.data_0[1][0], scene_0.view_0.data_0[2][0], scene_0.view_0.data_0[3][0], scene_0.view_0.data_0[0][1], scene_0.view_0.data_0[1][1], scene_0.view_0.data_0[2][1], scene_0.view_0.data_0[3][1], scene_0.view_0.data_0[0][2], scene_0.view_0.data_0[1][2], scene_0.view_0.data_0[2][2], scene_0.view_0.data_0[3][2], scene_0.view_0.data_0[0][3], scene_0.view_0.data_0[1][3], scene_0.view_0.data_0[2][3], scene_0.view_0.data_0[3][3]))))) * (mat4x4(scene_0.projection_0.data_0[0][0], scene_0.projection_0.data_0[1][0], scene_0.projection_0.data_0[2][0], scene_0.projection_0.data_0[3][0], scene_0.projection_0.data_0[0][1], scene_0.projection_0.data_0[1][1], scene_0.projection_0.data_0[2][1], scene_0.projection_0.data_0[3][1], scene_0.projection_0.data_0[0][2], scene_0.projection_0.data_0[1][2], scene_0.projection_0.data_0[2][2], scene_0.projection_0.data_0[3][2], scene_0.projection_0.data_0[0][3], scene_0.projection_0.data_0[1][3], scene_0.projection_0.data_0[2][3], scene_0.projection_0.data_0[3][3]))));
    output_0.normal_0 = input_normal_0;
    output_0.uv_0 = input_uv_0;
    VertexOutput_0 _S1 = output_0;

    #line 36
    gl_Position = output_0.position_0;

    #line 36
    entryPointParam_vertexMain_normal_0 = _S1.normal_0;

    #line 36
    entryPointParam_vertexMain_uv_0 = _S1.uv_0;

    #line 36
    return;
}
