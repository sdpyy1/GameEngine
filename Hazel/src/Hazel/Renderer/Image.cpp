#include "hzpch.h"
#include "Image.h"
#include "Hazel/Renderer/RendererAPI.h"
namespace Hazel {
	Ref_old<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer1 buffer)
	{
		HZ_CORE_ASSERT(!buffer);

		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::APIType::None: return nullptr;
			//case RendererAPI::APIType::Vulkan: return CreateRef<VulkanImage2D>(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
