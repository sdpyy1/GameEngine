#include "hzpch.h"
#include "RenderCommandBuffer.h"

#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanRenderCommandBuffer.h>

namespace Hazel {
	Ref<RenderCommandBuffer> RenderCommandBuffer::Create(const std::string& debugName, uint32_t count)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(count, debugName);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<RenderCommandBuffer> RenderCommandBuffer::CreateFromSwapChain(const std::string& debugName)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanRenderCommandBuffer>::Create(debugName, true);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
