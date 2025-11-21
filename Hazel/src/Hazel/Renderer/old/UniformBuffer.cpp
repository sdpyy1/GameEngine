#include "hzpch.h"
#include "UniformBuffer.h"

#include "Hazel/Renderer/old/Renderer.h"
#include <Hazel/Platform/Vulkan/VulkanUniformBuffer.h>

namespace GameEngine {

	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, std::string debugName)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:     return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanUniformBuffer>::Create(size,debugName);
		}

		ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
