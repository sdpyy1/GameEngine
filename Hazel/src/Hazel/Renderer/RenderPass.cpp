#include "hzpch.h"
#include "RenderPass.h"

#include "Renderer.h"

#include "Platform/Vulkan/VulkanRenderPass.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {
	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanRenderPass>::Create(spec);
		}

		ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
