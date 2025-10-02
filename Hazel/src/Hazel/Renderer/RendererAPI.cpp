#include "hzpch.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanRenderer.h>


namespace Hazel {

	RendererAPI::Type RendererAPI::s_API = RendererAPI::Type::Vulkan;

	Scope<RendererAPI> RendererAPI::Create_old()
	{
		switch (s_API)
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
			case RendererAPI::Type::Vulkan:  return CreateScope<VulkanRenderer>();
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
