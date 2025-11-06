#version 450 core
#include "include/Utils.glslh"
#include "include/Common.glslh"
#include "include/Buffer.glslh"
struct VertexOutput
{
	vec3 WorldPosition;
	vec3 Normal;
	vec2 TexCoord;
	mat3 WorldNormals;
	mat3 WorldTransform;
	vec3 Binormal;
	mat3 CameraView;
	vec3 ShadowMapCoords[4];
	vec3 ViewPosition;
};

#ifdef VERTEX_SHADER
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inTangent;
layout(location = 3) in vec3 inBinormal; 
layout(location = 4) in vec2 inTexCoord;

layout(location = 5) in vec4 a_MRow0;
layout(location = 6) in vec4 a_MRow1;
layout(location = 7) in vec4 a_MRow2;

layout(location = 0) out VertexOutput Output;

void main() {
    mat4 transform = mat4(
		vec4(a_MRow0.x, a_MRow1.x, a_MRow2.x, 0.0),
		vec4(a_MRow0.y, a_MRow1.y, a_MRow2.y, 0.0),
		vec4(a_MRow0.z, a_MRow1.z, a_MRow2.z, 0.0),
		vec4(a_MRow0.w, a_MRow1.w, a_MRow2.w, 1.0)
	);
    vec4 worldPosition = transform * vec4(inPosition, 1.0);

	Output.WorldPosition = worldPosition.xyz;
	Output.Normal = mat3(transform) * inNormal;
	Output.TexCoord = vec2(inTexCoord.x, inTexCoord.y);
	Output.WorldNormals = mat3(transform) * mat3(inTangent, inBinormal, inNormal);
	Output.WorldTransform = mat3(transform);
	Output.Binormal = inBinormal;
	Output.CameraView = mat3(u_CameraData.view);
	Output.ViewPosition = vec3(u_CameraData.view * vec4(Output.WorldPosition, 1.0));
	Output.ViewPosition.z *= -1;
	gl_Position = u_CameraData.viewProj * worldPosition;
}
#endif

#ifdef FRAGMENT_SHADER
layout(location = 0) in VertexOutput Input;

layout(set = 1, binding = 0) uniform sampler2D u_AlbedoTexture;
layout(set = 1, binding = 1) uniform sampler2D u_NormalTexture;
layout(set = 1, binding = 2) uniform sampler2D u_MetalnessTexture;
layout(set = 1, binding = 3) uniform sampler2D u_RoughnessTexture;
layout(set = 1, binding = 4) uniform sampler2D u_EmssiveTexture;


layout(push_constant) uniform Material
{
	vec3 AlbedoColor;
	float Metalness;
	vec3 Emission;
	float Roughness;
	uint UseNormalMap;
	uint padding[3];
} u_MaterialUniforms;

layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_MetalnessRoughness;
layout(location = 2) out vec4 o_Position;
layout(location = 3) out vec4 o_Normal;


// Used in PBR shader
struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
} m_Params;

void main() {
	vec4 albedoTexColor = texture(u_AlbedoTexture, Input.TexCoord);
   	o_Color = vec4(albedoTexColor.rgb * ToLinear(vec4(u_MaterialUniforms.AlbedoColor, 1.0)).rgb + u_MaterialUniforms.Emission * texture(u_EmssiveTexture,Input.TexCoord).rgb,1.0);   // MaterialUniforms.AlbedoColor is perceptual, must be converted to linear.
	m_Params.Metalness = texture(u_MetalnessTexture, Input.TexCoord).b * u_MaterialUniforms.Metalness;
	m_Params.Roughness = texture(u_RoughnessTexture, Input.TexCoord).g * u_MaterialUniforms.Roughness;
	o_MetalnessRoughness = vec4(m_Params.Metalness, m_Params.Roughness, 0.f, 1.f);
	o_Position = vec4(Input.WorldPosition, 1);
	m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(Input.Normal);		
	if (u_MaterialUniforms.UseNormalMap == 1)
	{
		m_Params.Normal = normalize(texture(u_NormalTexture, Input.TexCoord).rgb * 2.0f - 1.0f);
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal); // Transform from Tangent to World space
	}
	o_Normal = vec4(m_Params.Normal, 1.0);
	
}
#endif
