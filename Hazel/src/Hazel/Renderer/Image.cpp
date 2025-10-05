#include "hzpch.h"
#include "Image.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanImage.h>
namespace Hazel {
	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer buffer)
	{
		HZ_CORE_ASSERT(!buffer);

		switch (RendererAPI::Current())
		{
			case RendererAPI::Type::None: return nullptr;
			case RendererAPI::Type::Vulkan: return Ref<VulkanImage2D>::Create(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
