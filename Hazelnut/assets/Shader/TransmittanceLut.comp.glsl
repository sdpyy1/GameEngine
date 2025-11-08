#version 450 core
#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8

#include "include/SkyCommon.glslh"
layout(rgba32f, binding = 0) uniform writeonly image2D transmittanceLut;
layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;
void main()
{
 // 计算当前线程对应的图像坐标（确保不超出图像范围）
    ivec2 texelCoord = ivec2(gl_GlobalInvocationID.xy);
    
    // 获取图像的实际尺寸
    ivec2 lutSize = imageSize(transmittanceLut);
    
    // 边界检查：如果当前坐标超出图像范围则直接返回
    if (texelCoord.x >= lutSize.x || texelCoord.y >= lutSize.y)
        return;
    
    // 定义粉色（RGBA格式，范围0.0-1.0）
    vec4 pinkColor = vec4(1.0, 0.75, 0.8, 1.0);  // 调整RGB值可改变粉色深浅
    
    // 写入颜色到图像
    imageStore(transmittanceLut, texelCoord, pinkColor);
}

#endif


