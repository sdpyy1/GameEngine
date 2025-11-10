#version 450 core
#ifdef COMPUTE_SHADER
layout(set = 0,binding = 0) uniform CameraDataUniform {
    mat4 view;
    mat4 proj;
	mat4 viewProj;
	float width;
	float height;
	float Near;
	float Far;
	vec3 CameraPosition;
	float padding;
	mat4 InverseViewProj;
} u_CameraData;
layout(set=1,rgba32f, binding = 0) uniform writeonly image2D o_Texture;
layout(set=1,binding = 1) uniform sampler2D u_InputTexture;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

void main()
{
	mat4 a = u_CameraData.viewProj;
	vec2 uv = gl_GlobalInvocationID.xy / vec2(imageSize(o_Texture));
    imageStore(o_Texture, ivec2(gl_GlobalInvocationID.xy), texture(u_InputTexture, uv));
}





#endif
