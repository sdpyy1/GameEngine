#version 450 core
#include "include/Common.glslh"
#ifdef VERTEX_SHADER
vec3 kNdcPoints[3] = vec3[](
    vec3(-1.0, -1.0, 0.0), 
    vec3( 3.0, -1.0, 0.0), 
    vec3(-1.0,  3.0, 0.0) 
);
layout(location = 0) out vec2 TexCoord;
void main()
{
    gl_Position = vec4(kNdcPoints[gl_VertexIndex].xyz, 1.0);
    TexCoord = (kNdcPoints[gl_VertexIndex].xy + 1.0) * 0.5;
}
#endif

#ifdef FRAGMENT_SHADER


layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 o_Color;
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

layout(set = 0,binding = 1) uniform sampler2D u_AlbedoTexture;
layout(set = 0,binding = 2) uniform sampler2D u_MetallicRoughnessTexture;
layout(set = 0,binding = 3) uniform sampler2D u_PositionTexture;
layout(set = 0,binding = 4) uniform sampler2D u_NormalTexture;
layout(std140, set = 0, binding = 5) uniform RendererData
{
	vec4 CascadeSplits;
	float LightSize;
	int ShadowType;
	bool debugCSM;
} u_RendererData;
layout (std140, set = 0, binding = 6) uniform ShadowData 
{
	mat4 DirLightMatrices[4];
} u_DirShadow;

layout(std140, set = 0, binding = 7) uniform SceneData
{
	DirectionalLight DirectionalLights;
	float EnvironmentMapIntensity;
} u_Scene;

layout(set = 0, binding = 8) uniform sampler2DArray u_ShadowMapTexture;
layout(set = 0, binding = 9) uniform samplerCube u_EnvRadianceTex;
layout(set = 0, binding = 10) uniform samplerCube u_EnvIrradianceTex;
layout(set = 0, binding = 11) uniform sampler2D u_BRDFLUTTexture;
float GetDirShadowBias()
{
	const float MINIMUM_SHADOW_BIAS = 0.002;
	return MINIMUM_SHADOW_BIAS;
}
float HardShadows_DirectionalLight(sampler2DArray shadowMap, uint cascade, vec3 shadowCoords)
{
	float bias = GetDirShadowBias();
	float shadowMapDepth = texture(shadowMap, vec3(shadowCoords.xy * 0.5 + 0.5, cascade)).x;
	return step(shadowCoords.z, shadowMapDepth + bias);
}
struct PBRParameters
{
	vec3 Albedo;
	float Roughness;
	float Metalness;

	vec3 Normal;
	vec3 View;
	float NdotV;
} m_Params;
vec3 FresnelSchlickRoughness(vec3 F0, float cosTheta, float roughness)
{
	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}
float NdfGGX(float cosLh, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}
float GaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}
// Schlick-GGX approximation of geometric attenuation function using Smith's method.
float GaSchlickGGX(float cosLi, float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights.
	return GaSchlickG1(cosLi, k) * GaSchlickG1(NdotV, k);
}	
vec3 CalculateDirLights(vec3 F0)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < 1; i++) //Only one light for now
	{
		if (u_Scene.DirectionalLights.Multiplier == 0.0)
			continue;

		vec3 Li = normalize(-u_Scene.DirectionalLights.Direction);
		vec3 Lradiance = u_Scene.DirectionalLights.Radiance * u_Scene.DirectionalLights.Multiplier;
		vec3 Lh = normalize(Li + m_Params.View);

		// Calculate angles between surface normal and various light vectors.
		float cosLi = max(0.0, dot(m_Params.Normal, Li));
		float cosLh = max(0.0, dot(m_Params.Normal, Lh));

		vec3 F = FresnelSchlickRoughness(F0, max(0.0, dot(Lh, m_Params.View)), m_Params.Roughness);
		float D = NdfGGX(cosLh, m_Params.Roughness);
		float G = GaSchlickGGX(cosLi, m_Params.NdotV, m_Params.Roughness);

		vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
		vec3 diffuseBRDF = kd * m_Params.Albedo;

		// Cook-Torrance
		vec3 specularBRDF = (F * D * G) / max(Epsilon, 4.0 * cosLi * m_Params.NdotV);
		specularBRDF = clamp(specularBRDF, vec3(0.0f), vec3(10.0f));
		result += (diffuseBRDF + specularBRDF) * Lradiance * cosLi;
	}
	return result;
}
vec3 IBL(vec3 F0, vec3 Lr)
{
	vec3 irradiance = texture(u_EnvIrradianceTex, m_Params.Normal).rgb;
	vec3 F = FresnelSchlickRoughness(F0, m_Params.NdotV, m_Params.Roughness);
	vec3 kd = (1.0 - F) * (1.0 - m_Params.Metalness);
	vec3 diffuseIBL = m_Params.Albedo * irradiance;

	int envRadianceTexLevels = textureQueryLevels(u_EnvRadianceTex);
	vec3 specularIrradiance = textureLod(u_EnvRadianceTex, Lr, m_Params.Roughness * envRadianceTexLevels).rgb;

	vec2 specularBRDF = texture(u_BRDFLUTTexture, vec2(m_Params.NdotV, m_Params.Roughness)).rg;
	vec3 specularIBL = specularIrradiance * (F0 * specularBRDF.x + specularBRDF.y);

	return kd * diffuseIBL + specularIBL;
}
void main()
{
    vec3 WorldPosition = texture(u_PositionTexture, TexCoord).xyz;
	if (WorldPosition == vec3(0.0)) { // 这部分无模型，后续天空盒渲染
		o_Color = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	// 阴影
	float shadowScale = 1.0;
	uint cascadeIndex = 0;

	if(u_Scene.DirectionalLights.Radiance != vec3(0.0)){
		vec3 CameraPosition = u_CameraData.CameraPosition;
		float dis = length(WorldPosition - CameraPosition);
		for (uint i = 0; i < 4; i++)
		{
			if (dis < u_RendererData.CascadeSplits[i])
			{
				cascadeIndex = i;
				break;
			}
		}
		vec4 shadowCoords = u_DirShadow.DirLightMatrices[cascadeIndex] * vec4(WorldPosition, 1.0);
		vec3 shadowTex = shadowCoords.xyz / shadowCoords.w;
		shadowScale = HardShadows_DirectionalLight(u_ShadowMapTexture, cascadeIndex, shadowTex);
	}
		
	

	// 直接光照
	m_Params.Albedo = texture(u_AlbedoTexture, TexCoord).xyz;
	m_Params.Metalness = texture(u_MetallicRoughnessTexture, TexCoord).b;
    m_Params.Roughness = texture(u_MetallicRoughnessTexture, TexCoord).g;
    m_Params.Normal = texture(u_NormalTexture, TexCoord).xyz;
	m_Params.View = normalize(u_CameraData.CameraPosition - WorldPosition); 
	m_Params.NdotV = max(dot(m_Params.Normal, m_Params.View), 0.0);
	vec3 Lr = 2.0 * m_Params.NdotV * m_Params.Normal - m_Params.View;
	const vec3 Fdielectric = vec3(0.04);
	vec3 F0 = mix(Fdielectric, m_Params.Albedo, m_Params.Metalness);
	vec3 lightContribution = CalculateDirLights(F0) * shadowScale;
	
	// IBL
	vec3 iblContribution = IBL(F0, Lr) * u_Scene.EnvironmentMapIntensity;

	vec3 finalColor = lightContribution + iblContribution;

	o_Color = vec4(finalColor,1);
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
		o_Color = vec4(mix(o_Color.xyz,cascadeColor,0.5), 1.0);
	}
}
#endif
