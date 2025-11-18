#pragma once
#include <cstdint>
#include <string>
#include "Hazel/Renderer/RHI/RHIResource.h"
namespace GameEngine { 
    enum RDGPassType
    {
        RDG_PASS_TYPE_RENDER = 0,
        RDG_PASS_TYPE_COMPUTE,
        RDG_PASS_TYPE_RAY_TRACING,
        RDG_PASS_TYPE_PRESENT,
        RDG_PASS_TYPE_COPY,

        RDG_PASS_NODE_TYPE_MAX_ENUM,    //
    };

    enum RDGResourceType
    {
        RDG_RESOURCE_TYPE_TEXTURE = 0,
        RDG_RESOURCE_TYPE_BUFFER,

        RDG_RESOURCE_TYPE_MAX_ENUM,    //
    };

    struct RDGResourceHandle
    {

        uint32_t Index = UINT32_MAX;
        bool IsValid() const { return Index != UINT32_MAX; }
    };
    class RDGResource
    {
    public:
        RDGResource(RDGResourceType type) { m_Type = type; }
        RDGResourceType m_Type;
        virtual ~RDGResource() = default;

        std::string Name;

        bool bExternal = false;

        bool bProduced = false;
        bool bConsumed = false;

    };
    using RDGResourceRef = std::shared_ptr<RDGResource>;

    struct RDGTextureDesc
    {
        uint32_t Width        = 0;
        uint32_t Height       = 0;
        uint32_t Depth        = 1;
        uint32_t MipLevels    = 1;
        uint32_t ArraySize    = 1;

        uint32_t Format       = 0;
        bool bTransient       = false;
    };

    class RDGTexture : public RDGResource
    {
    public:
        RDGTexture(const RDGTextureDesc& InDesc, const std::string& InName) : RDGResource(RDG_RESOURCE_TYPE_TEXTURE)
        {
            Desc = InDesc;
            Name = InName;
        }
        RDGTextureDesc Desc;
        RHITextureRef m_RHITexture = nullptr;

    };
    using RDGTextureRef = std::shared_ptr<RDGTexture>;

    struct RDGBufferDesc
    {
        uint32_t Size = 0;       
        uint32_t UsageFlags = 0; 
        bool bTransient = false;
    };

    class RDGBuffer : public RDGResource
    {
    public:
        RDGBuffer(const RDGBufferDesc& InDesc, const std::string& InName): RDGResource(RDG_RESOURCE_TYPE_BUFFER) // 调用基类构造，标记类型
        {
            Desc = InDesc;
            Name = InName;
        }
        RDGBufferDesc Desc;

        RHIBufferRef m_RHIBuffer = nullptr;
    };

    using RDGBufferRef = std::shared_ptr<RDGBuffer>;
}

