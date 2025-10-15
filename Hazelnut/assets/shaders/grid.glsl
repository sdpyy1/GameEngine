#version 460 core
vec3 kNdcPoints[6] = vec3[](
    vec3( 1,  1, 0), 
    vec3(-1, -1, 0), 
    vec3(-1,  1, 0),
    vec3(-1, -1, 0), 
    vec3( 1,  1, 0), 
    vec3( 1, -1, 0)
);
layout(set=0,binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
	float width;
	float height;
} ubo;
// 描述符集
layout (set = 0, binding = 1) uniform sampler2D inDepth;

#ifdef VERTEX_SHADER
layout(location = 0) out vec3 nearPoint; 
layout(location = 1) out vec3 farPoint; 
vec3 deprojectNDC2World(vec2 pos, float z) 
{
    vec4 worldH = inverse(ubo.proj * ubo.view) * vec4(pos, z, 1.0);
    return worldH.xyz / worldH.w;
}
void main()
{
    nearPoint = deprojectNDC2World(kNdcPoints[gl_VertexIndex].xy, 1.0);
    farPoint  = deprojectNDC2World(kNdcPoints[gl_VertexIndex].xy, 0.0);

    gl_Position = vec4(kNdcPoints[gl_VertexIndex].xyz, 1.0);
}
#endif // VERTEX_SHADER

#ifdef FRAGMENT_SHADER
layout(location = 0) in vec3 nearPoint; 
layout(location = 1) in vec3 farPoint; 

layout(location = 0) out vec4 outColor;

// 判断坐标是否在指定范围
bool onRange(float val, float minVal, float maxVal)
{
    return val >= minVal && val <= maxVal;
}

vec4 grid(vec3 fragPos3D) 
{
    // 两层网格：大格+小格
    float gray0 = 0.3; // 大格
    float scale0 = 1.0;

    float gray1 = 0.6; // 小格
    float scale1 = 0.1;

    // 大格
    vec2 coord0 = fragPos3D.xz * scale0; 
    vec2 grid0 = abs(fract(coord0 - 0.5) - 0.5) / fwidth(coord0 * 2.0);
    float line0 = min(grid0.x, grid0.y);
    vec4 color = vec4(gray0, gray0, gray0, 1.0 - min(line0, 1.0));

    // 小格
    vec2 coord1 = fragPos3D.xz * scale1; 
    vec2 grid1 = abs(fract(coord1 - 0.5) - 0.5) / fwidth(coord1 * 2.0);
    float line1 = min(grid1.x, grid1.y);
    float a1 = 1.0 - min(line1, 1.0);
    if(a1 > 0.0)
        color = mix(color, vec4(gray1, gray1, gray1, a1), a1);

    // 中心轴高亮（X/Z=0线）
    vec2 coord = fragPos3D.xz;
    bool xDraw = onRange(coord.x, -0.5, 0.5);
    bool zDraw = onRange(coord.y, -0.5, 0.5);
    if(xDraw || zDraw)
    {
        vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord * 2.0);
        vec2 a = vec2(1.0) - min(grid, vec2(1.0));

        if(xDraw)
            color.xyz = mix(color.xyz, vec3(1.0, 0.12, 0.18), a.x);
        if(zDraw)
            color.xyz = mix(color.xyz, vec3(0.1, 0.3, 1.0), a.y);
    }

    return color;
}

float computeDepth(vec3 pos) 
{
    vec4 posH = inverse(ubo.proj * ubo.view) * vec4(pos, 1.0);
    return posH.z / posH.w;
}
float linearizeDepth(float z, float n, float f)
{
    return n * f / (z * (f - n) + n);
}

vec4 getColor(vec3 fragPos3D, float t)
{
    float deviceZ = computeDepth(fragPos3D);

    vec2 uv = gl_FragCoord.xy / vec2(ubo.width, ubo.height);
    float sceneZ = texture(inDepth,uv).r;

    float linearDepth = linearizeDepth(deviceZ,0.1,1000); // 转线性
    float fading = exp2(-linearDepth * 0.05);

    vec4 result = grid(fragPos3D) * float(t > 0);
    result.a = (deviceZ > sceneZ) ? result.a : 0.0;
    result.a *= fading * 0.75;

    return result;
}

void main()
{
    float t = -nearPoint.y / (farPoint.y - nearPoint.y);
    vec3 fragPos3D = nearPoint + t * (farPoint - nearPoint);
    outColor = getColor(fragPos3D, t);
}

#endif // PIXEL_SHADER
