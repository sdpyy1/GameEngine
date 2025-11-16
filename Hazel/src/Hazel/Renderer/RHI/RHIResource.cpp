#include "hzpch.h"
#include "RHIResource.h"
namespace Hazel {

	Extent3D RHITexture::MipExtent(uint32_t mipLevel)
	{
        if (mipLevel > info.mipLevels) LOG_WARN("Mip level is greater than texture`s max mip!");

        Extent3D extent = info.extent;
        extent.width = std::max((uint32_t)1, extent.width >> mipLevel);
        extent.height = std::max((uint32_t)1, extent.height >> mipLevel);
        extent.depth = std::max((uint32_t)1, extent.depth >> mipLevel);

        return extent;
	}

    RHICommandListRef RHICommandPool::CreateCommandList(bool byPass)
    {
        return nullptr;
    }

}