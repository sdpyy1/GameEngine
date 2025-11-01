#version 450 core
#include "include/Utils.glslh"
#include "include/Common.glslh"
layout(set = 0,binding = 0) uniform CameraDataUniform {
    mat4 view;
    mat4 proj;
	float width;
	float height;
	float Near;
	float Far;
	vec3 CameraPosition;
} u_CameraData;
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
// Bone influences
layout(location = 8) in ivec4 a_BoneIndices;
layout(location = 9) in vec4 a_BoneWeights;
layout (std140, set = 0, binding = 1) readonly buffer BoneTransforms
{
	mat4 BoneTransforms[MAX_BONES * MAX_ANIMATED_MESHES];
} r_BoneTransforms;

layout(push_constant) uniform BoneTransformIndex
{
	layout(offset=64) uint Base;
} u_BoneTransformIndex;
layout (std140, set = 0, binding = 2) uniform ShadowData 
{
	mat4 DirLightMatrices[4];
} u_DirShadow;

layout(location = 0) out VertexOutput Output;


void main() {
    mat4 transform = mat4(
		vec4(a_MRow0.x, a_MRow1.x, a_MRow2.x, 0.0),
		vec4(a_MRow0.y, a_MRow1.y, a_MRow2.y, 0.0),
		vec4(a_MRow0.z, a_MRow1.z, a_MRow2.z, 0.0),
		vec4(a_MRow0.w, a_MRow1.w, a_MRow2.w, 1.0)
	);
	mat4 boneTransform = r_BoneTransforms.BoneTransforms[(u_BoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + a_BoneIndices[0]] * a_BoneWeights[0];
	boneTransform     += r_BoneTransforms.BoneTransforms[(u_BoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + a_BoneIndices[1]] * a_BoneWeights[1];
	boneTransform     += r_BoneTransforms.BoneTransforms[(u_BoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + a_BoneIndices[2]] * a_BoneWeights[2];
	boneTransform     += r_BoneTransforms.BoneTransforms[(u_BoneTransformIndex.Base + gl_InstanceIndex) * MAX_BONES + a_BoneIndices[3]] * a_BoneWeights[3];

vec4 worldPosition = transform * boneTransform * vec4(inPosition, 1.0);

	Output.WorldPosition = worldPosition.xyz;
	Output.Normal = mat3(transform) * mat3(boneTransform) * inNormal;
	Output.TexCoord = vec2(inTexCoord.x, inTexCoord.y);
	Output.WorldNormals = mat3(transform) * mat3(boneTransform) * mat3(inTangent, inBinormal, inNormal);
	Output.WorldTransform = mat3(transform) * mat3(boneTransform);
	Output.Binormal = mat3(transform) * mat3(boneTransform) * inBinormal;

	Output.CameraView = mat3(u_CameraData.view);

	vec4 shadowCoords[4];
	shadowCoords[0] = u_DirShadow.DirLightMatrices[0] * vec4(Output.WorldPosition.xyz, 1.0);
	shadowCoords[1] = u_DirShadow.DirLightMatrices[1] * vec4(Output.WorldPosition.xyz, 1.0);
	shadowCoords[2] = u_DirShadow.DirLightMatrices[2] * vec4(Output.WorldPosition.xyz, 1.0);
	shadowCoords[3] = u_DirShadow.DirLightMatrices[3] * vec4(Output.WorldPosition.xyz, 1.0);
	Output.ShadowMapCoords[0] = vec3(shadowCoords[0].xyz / shadowCoords[0].w);
	Output.ShadowMapCoords[1] = vec3(shadowCoords[1].xyz / shadowCoords[1].w);
	Output.ShadowMapCoords[2] = vec3(shadowCoords[2].xyz / shadowCoords[2].w);
	Output.ShadowMapCoords[3] = vec3(shadowCoords[3].xyz / shadowCoords[3].w);

	Output.ViewPosition = vec3(u_CameraData.view * vec4(Output.WorldPosition, 1.0));

	gl_Position = u_CameraData.proj * u_CameraData.view * worldPosition;
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
	float Roughness;
	float Emission;

	bool UseNormalMap;
} u_MaterialUniforms;


layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_MetalnessRoughness;


layout(std140, set = 0, binding = 3) uniform RendererData
{
	uniform vec4 CascadeSplits;
	uniform float LightSize;
} u_RendererData;
layout(std140, set = 0, binding = 4) uniform SceneData
{
	DirectionalLight DirectionalLights;
	float EnvironmentMapIntensity;
} u_Scene;
layout(set = 0, binding = 5) uniform sampler2DArray u_ShadowMapTexture;
layout(set = 0, binding = 6) uniform samplerCube u_EnvRadianceTex;
layout(set = 0, binding = 7) uniform samplerCube u_EnvIrradianceTex;
layout(set = 0, binding = 8) uniform sampler2D u_BRDFLUTTexture;
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
#include "include/GBufferUtils.glslh"

void main() {
	vec4 albedoTexColor = texture(u_AlbedoTexture, Input.TexCoord);
   	m_Params.Albedo = albedoTexColor.rgb * ToLinear(vec4(u_MaterialUniforms.AlbedoColor, 1.0)).rgb;   // MaterialUniforms.AlbedoColor is perceptual, must be converted to linear.
	float alpha = albedoTexColor.a;
	m_Params.Metalness = texture(u_MetalnessTexture, Input.TexCoord).b * u_MaterialUniforms.Metalness;
	m_Params.Roughness = texture(u_RoughnessTexture, Input.TexCoord).g * u_MaterialUniforms.Roughness;
	o_MetalnessRoughness = vec4(m_Params.Metalness, m_Params.Roughness, 0.f, 1.f);
	m_Params.Roughness = max(m_Params.Roughness, 0.05); // Minimum roughness of 0.05 to keep specular highlight

	// Normals (either from vertex or map)
	m_Params.Normal = normalize(Input.Normal);
	if (u_MaterialUniforms.UseNormalMap)
	{
		m_Params.Normal = normalize(texture(u_NormalTexture, Input.TexCoord).rgb * 2.0f - 1.0f);
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal);
	}
	m_Params.View = normalize(u_CameraData.CameraPosition - Input.WorldPosition);
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	// Specular reflection vector
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	uint cascadeIndex = 0;
	const uint SHADOW_MAP_CASCADE_COUNT = 4;
	for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT - 1; i++)
	{
		if (Input.ViewPosition.z < u_RendererData.CascadeSplits[i])
			cascadeIndex = i + 1;
	}


	float shadowScale;

	vec3 shadowMapCoords = GetShadowMapCoords(Input.ShadowMapCoords, cascadeIndex);
	shadowScale = true ? PCSS_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_RendererData.LightSize) : HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);


	shadowScale = 1.0 - clamp(u_Scene.DirectionalLights.ShadowAmount - shadowScale, 0.0f, 1.0f);

	// Direct lighting
	vec3 lightContribution = CalculateDirLights(F0) * shadowScale;
	// lightContribution += CalculatePointLights(F0, Input.WorldPosition);
	// lightContribution += CalculateSpotLights(F0, Input.WorldPosition) * SpotShadowCalculation(u_SpotShadowTexture, Input.WorldPosition);
	lightContribution += m_Params.Albedo * u_MaterialUniforms.Emission;

	// Indirect lighting
	vec3 iblContribution = IBL(F0, Lr) * u_Scene.EnvironmentMapIntensity;

	// Final color
	o_Color = vec4(iblContribution + lightContribution, 1.0);
}
#endif
