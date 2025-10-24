#include "hzpch.h"
#include "UniformBufferSet.h"

#include "Hazel/Renderer/Renderer.h"

#include "Platform/Vulkan/VulkanUniformBufferSet.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<UniformBufferSet> UniformBufferSet::Create(uint32_t size,std::string debugName, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:   return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanUniformBufferSet>::Create(size, debugName, framesInFlight);
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
