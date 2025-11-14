#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Hazel/Platform/Vulkan/VulkanRenderer.h>

namespace Hazel {
	RendererAPI::Type RendererAPI::s_API = RendererAPI::Type::Vulkan;

	RendererAPI* RendererAPI::CreateAPI()
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::Vulkan: return new VulkanRenderer();
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
