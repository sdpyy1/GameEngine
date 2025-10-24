#include "hzpch.h"
#include "UniformBuffer.h"

#include "Hazel/Renderer/Renderer.h"
#include <Platform/Vulkan/VulkanUniformBuffer.h>

namespace Hazel {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, std::string debugName)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:     return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size,debugName);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
