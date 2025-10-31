#include "hzpch.h"
#include "Pipeline.h"

#include "Renderer.h"

#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanPipeline.h>

namespace Hazel {
	Ref<Pipeline> Pipeline::Create(const PipelineSpecification& spec)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None:    return nullptr;
		case RendererAPI::Type::Vulkan:  return Ref<VulkanPipeline>::Create(spec);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
