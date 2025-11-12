#include "hzpch.h"
#include "ComputePass.h"
#include "Platform/Vulkan/VulkanComputePass.h"
#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel
{
	Ref<Hazel::ComputePass> Hazel::ComputePass::Create(const ComputePassSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanComputePass>::Create(spec);
		}

		ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}
}
