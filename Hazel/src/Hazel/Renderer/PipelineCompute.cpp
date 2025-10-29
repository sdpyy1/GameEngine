#include "hzpch.h"
#include "PipelineCompute.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <Platform/Vulkan/VulkanComputePipeline.h>
namespace Hazel {
	Ref<PipelineCompute> PipelineCompute::Create(Ref<Shader> computeShader)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanComputePipeline>::Create(computeShader);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
