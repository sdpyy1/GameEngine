#version 450 core
#include "include/Buffer.glslh"
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
		vec3 shadowMapCoords = shadowTex;
		if(u_RendererData.ShadowType == 0) shadowScale = HardShadows_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords);
		else if(u_RendererData.ShadowType == 1) shadowScale = PCF_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords,u_RendererData.LightSize);
		else if(u_RendererData.ShadowType == 2) shadowScale = PCSS_DirectionalLight(u_DirShadowMapTexture, cascadeIndex, shadowMapCoords, u_RendererData.LightSize);
	}

	// 直接光照
	m_Params.Albedo = texture(u_AlbedoTexture, TexCoord).xyz;
	m_Params.Metalness = texture(u_MRTexture, TexCoord).b;
    m_Params.Roughness = texture(u_MRTexture, TexCoord).g;
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
	if(u_RendererData.debugCSM == 1)
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
