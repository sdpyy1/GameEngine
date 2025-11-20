#include "hzpch.h"
#include "PipelineCompute.h"
#include "Hazel/Renderer/old/RendererAPI.h"
#include <Hazel/Platform/Vulkan/VulkanComputePipeline.h>
namespace GameEngine {
	Ref<PipelineCompute> PipelineCompute::Create(Ref<Shader> computeShader)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanComputePipeline>::Create(computeShader);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
