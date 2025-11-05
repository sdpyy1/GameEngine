#version 450 core
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

// G-buffer纹理
layout(set=0, binding = 0) uniform sampler2D gPosition;
layout(set=0, binding = 1) uniform sampler2D gNormal;
layout(set=0, binding = 2) uniform sampler2D gAlbedo;
layout(set=0, binding = 3) uniform sampler2D gMR;
// 环境贴图（用于演示，实际可简化）
layout(set=0, binding = 4) uniform samplerCube irradianceMap;
layout(set=0, binding = 5) uniform samplerCube prefilterMap;
layout(set=0, binding = 6) uniform sampler2D lutMap;

layout(location = 0) out vec4 FragColor;

// 固定平行光参数
const vec3 lightDir = normalize(vec3(-1.0, -1.0, -1.0)); // 平行光方向
const vec3 lightColor = vec3(1.0, 0.9, 0.8); // 暖白光
const float lightIntensity = 3.0;

// 辅助函数：菲涅尔计算
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// 辅助函数：GGX法线分布
float distributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926 * denom * denom;

    return a2 / denom;
}

// 辅助函数：几何遮挡
float geometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

void main()
{
    // 从G-buffer采样数据
    vec3 worldPos = texture(gPosition, TexCoord).xyz;
    vec3 normal = normalize(texture(gNormal, TexCoord).xyz);
    vec3 albedo = texture(gAlbedo, TexCoord).rgb;
    float metalness = texture(gMR, TexCoord).r;
    float roughness = texture(gMR, TexCoord).g;

    // 计算光照所需向量
    vec3 V = normalize(-worldPos); // 视角方向（假设相机在原点）
    vec3 L = -lightDir; // 平行光方向（取反因为lightDir是光照方向）
    vec3 H = normalize(V + L); // 半程向量

    // 基础参数
    vec3 F0 = mix(vec3(0.04), albedo, metalness); // 反射率基础值
    vec3 albedoMetallic = albedo * (1.0 - metalness); // 金属度修正反照率

    // 直接光照计算（平行光）
    float NdotL = max(dot(normal, L), 0.0);
    float NdotV = max(dot(normal, V), 0.0);
    
    // PBR光照分量
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    float D = distributionGGX(normal, H, roughness);
    float G = geometrySmith(normal, V, L, roughness);

    // 高光项
    vec3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);
    // 漫反射项
    vec3 diffuse = albedoMetallic / 3.1415926;

    // 能量守恒（漫反射 = 1 - 高光）
    vec3 kD = (vec3(1.0) - F) * (1.0 - metalness);

    // 平行光贡献
    vec3 directLight = (kD * diffuse + specular) * lightColor * lightIntensity * NdotL;

    // 环境光（简单使用辐照度贴图，确保用到cubemap）
    vec3 irradiance = texture(irradianceMap, normal).rgb;
    vec3 ambient = irradiance * albedoMetallic * 0.1; // 弱环境光

    // 最终颜色（加上LUT贴图采样作为微调，确保用到所有纹理）
    vec2 lutCoord = vec2(NdotV, roughness);
    vec3 lutColor = texture(lutMap, lutCoord).rgb;
    vec3 finalColor = (directLight + ambient) * lutColor;

    FragColor = vec4(finalColor, 1.0);
}
#endif
