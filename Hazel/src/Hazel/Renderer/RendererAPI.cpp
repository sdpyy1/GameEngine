#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanRenderer.h>


namespace Hazel {

	RendererAPI::Type RendererAPI::s_API = RendererAPI::Type::Vulkan;

	RendererAPI* RendererAPI::CreateAPI()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::Vulkan: return new VulkanRenderer();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
