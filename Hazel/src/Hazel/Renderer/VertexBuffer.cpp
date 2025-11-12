#include "hzpch.h"
#include "VertexBuffer.h"
#include "hzpch.h"
#include "VertexBuffer.h"

#include "Renderer.h"

#include "Platform/Vulkan/VulkanVertexBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<VertexBuffer> VertexBuffer::Create(void* data, uint64_t size,std::string debugeName, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanVertexBuffer>::Create(data, size, debugeName,usage);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(uint64_t size, std::string debugeName, VertexBufferUsage usage)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanVertexBuffer>::Create(size,debugeName, usage);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
