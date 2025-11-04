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
	float Roughness;
	vec3 Emission;

	bool UseNormalMap;
} u_MaterialUniforms;


layout(location = 0) out vec4 o_Color;
layout(location = 1) out vec4 o_MetalnessRoughness;


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
#include "include/PBR.glslh"

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
		m_Params.Normal = normalize(Input.WorldNormals * m_Params.Normal); // Transform from Tangent to World space
	}
	m_Params.View = normalize(u_CameraData.CameraPosition - Input.WorldPosition); 
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	// Specular reflection vector
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);

	uint cascadeIndex = 3;
	const uint SHADOW_MAP_CASCADE_COUNT = 4;

		for (uint i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			if (Input.ViewPosition.z < u_RendererData.CascadeSplits[i])
			{
				cascadeIndex = i;
				break;  // 找到第一个匹配就退出
			}
		}
	// shadowScale：可见性
	float shadowScale;
	vec3 shadowMapCoords = GetShadowMapCoords(Input.ShadowMapCoords, cascadeIndex);
	if(u_RendererData.ShadowType == 0) shadowScale = HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords);
	else if(u_RendererData.ShadowType == 1) shadowScale = PCF_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, 3);
	else if(u_RendererData.ShadowType == 2) shadowScale = PCSS_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowMapCoords, u_RendererData.LightSize);
		

	// Direct lighting
	vec3 lightContribution = CalculateDirLights(F0) * shadowScale;
	lightContribution += u_MaterialUniforms.Emission + texture(u_EmssiveTexture,Input.TexCoord).rgb;

	// Indirect lighting
	vec3 iblContribution = IBL(F0, Lr) * u_Scene.EnvironmentMapIntensity;

	vec3 finalColor = lightContribution + iblContribution;
	
	// Final color
    o_Color = vec4(finalColor, 1.0);


	// Debug
	if(u_RendererData.debugCSM)
	{
		vec3 cascadeColor;
			switch(cascadeIndex) {
			case 0: cascadeColor = vec3(1.0, 0.0, 0.0); break; // 红色 - 级联0
			case 1: cascadeColor = vec3(0.0, 1.0, 0.0); break; // 绿色 - 级联1
			case 2: cascadeColor = vec3(0.0, 0.0, 1.0); break; // 蓝色 - 级联2
			case 3: cascadeColor = vec3(1.0, 1.0, 0.0); break; // 黄色 - 级联3
			default: cascadeColor = vec3(1.0, 0.0, 1.0); // 紫色 - 异常
		}
		o_Color = vec4(mix(finalColor,cascadeColor,0.5), 1.0);
	}
}
#endif
