#version 450 core
// 启用非统一索引扩展（关键：解决变量索引访问变量索引的问题）
#extension GL_EXT_nonuniform_qualifier : require

#ifdef COMPUTE_SHADER
#define LOCAL_SIZE 8

// 输入：原始深度图（采样器）
layout(binding = 0) uniform sampler2D u_InputDepth;
// 输出：HZB所有层级（图像数组，注意：writeonly需保持一致）
layout(binding = 1, r32f) uniform image2D o_HZB[11]; // 移除writeonly，避免传递时的限定符冲突

layout(push_constant) uniform PushConstants {
    uint mipLevels;
};

layout(local_size_x = LOCAL_SIZE, local_size_y = LOCAL_SIZE, local_size_z = 1) in;

void main()
{
    ivec2 globalCoord = ivec2(gl_GlobalInvocationID.xy);

    // 第一步：复制原始深度图到HZB第0级
    {
        ivec2 level0Size = imageSize(o_HZB[0]);
        if (globalCoord.x < level0Size.x && globalCoord.y < level0Size.y) {
            float depthVal = texelFetch(u_InputDepth, globalCoord, 0).r;
            // 显式指定内存操作限定符（与图像布局匹配）
            imageStore(o_HZB[0], globalCoord, vec4(depthVal));
        }
    }

    // 第二步：计算第1级及以上
    for (uint mip = 1; mip < mipLevels; mip++) {
        // 使用非统一索引访问数组（需扩展支持）
        ivec2 mipSize = imageSize(o_HZB[nonuniformEXT(mip)]);
        ivec2 mipCoord = globalCoord >> (mip - 1);

        if (mipCoord.x >= mipSize.x || mipCoord.y >= mipSize.y)
            continue;

        ivec2 srcCoord = mipCoord * 2;
        ivec2 srcSize = imageSize(o_HZB[nonuniformEXT(mip - 1)]);

        // 访问上一级图像时使用nonuniformEXT
        float color00 = imageLoad(o_HZB[nonuniformEXT(mip - 1)], srcCoord).r;
        float color01 = (srcCoord.x + 1 < srcSize.x) ? imageLoad(o_HZB[nonuniformEXT(mip - 1)], srcCoord + ivec2(1, 0)).r : color00;
        float color10 = (srcCoord.y + 1 < srcSize.y) ? imageLoad(o_HZB[nonuniformEXT(mip - 1)], srcCoord + ivec2(0, 1)).r : color00;
        float color11 = (srcCoord.x + 1 < srcSize.x && srcCoord.y + 1 < srcSize.y) 
            ? imageLoad(o_HZB[nonuniformEXT(mip - 1)], srcCoord + ivec2(1, 1)).r 
            : color00;

        float minVal = min(min(color00, color01), min(color10, color11));
        // 写入当前级时使用nonuniformEXT
        imageStore(o_HZB[nonuniformEXT(mip)], mipCoord, vec4(minVal));
    }
}
#endif
