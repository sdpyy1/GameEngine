#include "hzpch.h"
#include "UniformBuffer.h"

#include "Hazel/Renderer/Renderer.h"
#include <Platform/Vulkan/VulkanUniformBuffer.h>

namespace Hazel {

	Ref_old<UniformBuffer> UniformBuffer::Create_old(uint32_t size, uint32_t binding)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:     return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
