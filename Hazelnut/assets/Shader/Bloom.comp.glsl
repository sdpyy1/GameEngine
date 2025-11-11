#version 450 core
#ifdef COMPUTE_SHADER
#include "include/Common.glslh"
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

layout(set = 1,rgba32f, binding = 0) uniform writeonly image2D o_Texture;
layout(set = 1,binding = 1) uniform sampler2D u_InputTexture;
layout(push_constant) uniform Uniforms
{
    vec4 Params; // (x) threshold, (y) threshold - knee, (z) knee * 2, (w) 0.25 / knee
    float LOD;
    int Mode;
} u_Uniforms;
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
#define MODE_PREFILTER      0
#define MODE_DOWNSAMPLE     1
#define MODE_UPSAMPLE_FIRST 2
#define MODE_UPSAMPLE       3

// 卷积
vec3 DownsampleBox13(sampler2D tex, float lod, vec2 uv, vec2 texelSize)
{
    // Center
    vec3 A = textureLod(tex, uv, lod).rgb;

    texelSize *= 0.5f; // Sample from center of texels

    // Inner box
    vec3 B = textureLod(tex, uv + texelSize * vec2(-1.0f, -1.0f), lod).rgb;
    vec3 C = textureLod(tex, uv + texelSize * vec2(-1.0f, 1.0f), lod).rgb;
    vec3 D = textureLod(tex, uv + texelSize * vec2(1.0f, 1.0f), lod).rgb;
    vec3 E = textureLod(tex, uv + texelSize * vec2(1.0f, -1.0f), lod).rgb;

    // Outer box
    vec3 F = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 G = textureLod(tex, uv + texelSize * vec2(-2.0f, 0.0f), lod).rgb;
    vec3 H = textureLod(tex, uv + texelSize * vec2(0.0f, 2.0f), lod).rgb;
    vec3 I = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 J = textureLod(tex, uv + texelSize * vec2(2.0f, 2.0f), lod).rgb;
    vec3 K = textureLod(tex, uv + texelSize * vec2(2.0f, 0.0f), lod).rgb;
    vec3 L = textureLod(tex, uv + texelSize * vec2(-2.0f, -2.0f), lod).rgb;
    vec3 M = textureLod(tex, uv + texelSize * vec2(0.0f, -2.0f), lod).rgb;

    // Weights
    vec3 result = vec3(0.0);
    // Inner box
    result += (B + C + D + E) * 0.5f;
    // Bottom-left box
    result += (F + G + A + M) * 0.125f;
    // Top-left box
    result += (G + H + I + A) * 0.125f;
    // Top-right box
    result += (A + I + J + K) * 0.125f;
    // Bottom-right box
    result += (M + A + K + L) * 0.125f;

    // 4 samples each
    result *= 0.25f;

    return result;
}
// Quadratic color thresholding
// curve = (threshold - knee, knee * 2, 0.25 / knee)
vec4 QuadraticThreshold(vec4 color, float threshold, vec3 curve)
{
    // Maximum pixel brightness
    float brightness = max(max(color.r, color.g), color.b);
    // Quadratic curve
    float rq = clamp(brightness - curve.x, 0.0, curve.y);
    rq = (rq * rq) * curve.z;
    color *= max(rq, brightness - threshold) / max(brightness, Epsilon);
    return color;
}
vec4 Prefilter(vec4 color, vec2 uv)
{
    float clampValue = 20.0f;
    color = clamp(color, vec4(0.0f), vec4(clampValue));
    color = QuadraticThreshold(color, u_Uniforms.Params.x, u_Uniforms.Params.yzw);
    return color;
}

vec3 UpsampleTent9(sampler2D tex, float lod, vec2 uv, vec2 texelSize, float radius)
{
    vec4 offset = texelSize.xyxy * vec4(1.0f, 1.0f, -1.0f, 0.0f) * radius;

    // Center
    vec3 result = textureLod(tex, uv, lod).rgb * 4.0f;

    result += textureLod(tex, uv - offset.xy, lod).rgb;
    result += textureLod(tex, uv - offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv - offset.zy, lod).rgb;

    result += textureLod(tex, uv + offset.zw, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xw, lod).rgb * 2.0;

    result += textureLod(tex, uv + offset.zy, lod).rgb;
    result += textureLod(tex, uv + offset.wy, lod).rgb * 2.0;
    result += textureLod(tex, uv + offset.xy, lod).rgb;

    return result * (1.0f / 16.0f);
}
void main()
{
	vec2 imgSize = vec2(imageSize(o_Texture));

    ivec2 invocID = ivec2(gl_GlobalInvocationID);
	if (invocID.x >= imgSize.x || invocID.y >= imgSize.y) return;
    vec2 texCoords = vec2(float(invocID.x) / imgSize.x, float(invocID.y) / imgSize.y);
    texCoords += (1.0f / imgSize) * 0.5f; // 获得像素中心的坐标

    vec2 texSize = vec2(textureSize(u_InputTexture, int(u_Uniforms.LOD)));
    vec4 color = vec4(0, 0, 0, 1);
    if (u_Uniforms.Mode == MODE_PREFILTER)
    {
        color.rgb = DownsampleBox13(u_InputTexture, 0, texCoords, 1.0f / texSize);
        color = Prefilter(color, texCoords);
        color.a = 1.0f;
    }else if (u_Uniforms.Mode == MODE_DOWNSAMPLE)
    {
        color.rgb = DownsampleBox13(u_InputTexture, u_Uniforms.LOD, texCoords, 1.0f / texSize);
    }else if (u_Uniforms.Mode == MODE_UPSAMPLE)
    {
		vec2 bloomTexSize = vec2(textureSize(u_InputTexture, int(u_Uniforms.LOD + 1.0f))); // 下一级的尺寸
        float sampleScale = 1.0f;
        vec3 upsampledTexture = UpsampleTent9(u_InputTexture, u_Uniforms.LOD + 1.0f, texCoords, 1.0f / bloomTexSize, sampleScale);

        vec3 existing = textureLod(u_InputTexture, texCoords, u_Uniforms.LOD).rgb;
        color.rgb = existing + upsampledTexture;
    }
    
	imageStore(o_Texture, ivec2(gl_GlobalInvocationID), color);

}





#endif
