#version 450 core
#ifdef VERTEX_SHADER
layout(set = 0,binding = 0) uniform CameraDataUniform {
    mat4 view;
    mat4 proj;
	mat4 viewProj;
	float width;
	float height;
	float Near;
	float Far;
	vec3 CameraPosition;
} u_CameraData;
vec2 NDC[3] = vec2[](
    vec2(-1.0, -1.0), 
    vec2( 3.0, -1.0), 
    vec2(-1.0,  3.0) 
);
layout(location = 0) out vec2 out_texCoord;
layout(location = 1) out vec3 v_dir;
void main(){
	vec4 position = vec4(NDC[gl_VertexIndex], 1.0, 1.0);
	gl_Position = position;
    out_texCoord = (NDC[gl_VertexIndex] + 1.0) * 0.5;
	v_dir = (inverse(u_CameraData.viewProj) * position).xyz;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec2 in_texCoord;
layout(location = 1) in vec3 v_dir;
layout(location = 0) out vec4 out_color;
layout(set=0,binding = 1) uniform samplerCube SkyTexture;
void main(){
	out_color = texture(SkyTexture, v_dir);
}
#endif
