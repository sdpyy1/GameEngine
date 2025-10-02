#include "hzpch.h"
#include "StorageBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanStorageBuffer.h>

namespace Hazel {

	Ref<StorageBuffer> StorageBuffer::Create(uint32_t size, const StorageBufferSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:     return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanStorageBuffer>::Create(size, specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

}
