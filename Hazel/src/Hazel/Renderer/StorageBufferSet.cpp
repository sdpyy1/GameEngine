#include "hzpch.h"

#include "UniformBufferSet.h"

#include "Hazel/Renderer/Renderer.h"

#include "StorageBufferSet.h"

#include "Hazel/Platform/Vulkan/VulkanStorageBufferSet.h"
#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<StorageBufferSet> StorageBufferSet::Create(const StorageBufferSpecification& specification, uint32_t size, uint32_t framesInFlight)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:   return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanStorageBufferSet>::Create(specification, size, framesInFlight);
		}

		ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
