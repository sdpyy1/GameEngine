#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8

layout(binding = 0) uniform sampler2D u_InputDepth;
layout(binding = 1, r32f) writeonly uniform image2D o_HZB;

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
     // 当前 Mip 级的像素坐标（全局线程 ID）
    ivec2 mipCoord = ivec2(gl_GlobalInvocationID.xy);
    // 当前 Mip 级的尺寸
    ivec2 mipSize = imageSize(o_HZB);
    
    // 越界检查
    if (mipCoord.x >= mipSize.x || mipCoord.y >= mipSize.y)
        return;
    
    // 上一级 Mip 的对应 2x2 区域坐标（因尺寸是当前级的 2 倍）
    ivec2 srcCoord = mipCoord * 2;  // 左上角
    
    // 采样上一级 2x2 区域的像素（双线性滤波或取最大值/最小值）
    float color00 = texelFetch(u_InputDepth, srcCoord, 0).r;
    float color01 = texelFetch(u_InputDepth, srcCoord + ivec2(1, 0), 0).r;
    float color10 = texelFetch(u_InputDepth, srcCoord + ivec2(0, 1), 0).r;
    float color11 = texelFetch(u_InputDepth, srcCoord + ivec2(1, 1), 0).r;
    
    // 下采样计算（示例：取平均值）
	float minVal = min(min(color00, color01), min(color10, color11));
    
    // 写入当前 Mip 级
    imageStore(o_HZB, mipCoord, vec4(minVal));
}
#endif
