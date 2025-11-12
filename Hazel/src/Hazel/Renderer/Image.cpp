#include "hzpch.h"
#include "Image.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanImage.h>
namespace Hazel {
	Ref<Image2D> Image2D::Create(const ImageSpecification& specification, Buffer buffer)
	{
		ASSERT(!buffer);

		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanImage2D>::Create(specification);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Hazel::Ref<Hazel::ImageView> ImageView::Create(const ImageViewSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanImageView>::Create(specification);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
