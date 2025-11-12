#include "hzpch.h"
#include "IndexBuffer.h"

#include "Renderer.h"

#include "Platform/Vulkan/VulkanIndexBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<IndexBuffer> IndexBuffer::Create(uint64_t size)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanIndexBuffer>::Create(size);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(void* data, uint64_t size)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanIndexBuffer>::Create(data, size);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
