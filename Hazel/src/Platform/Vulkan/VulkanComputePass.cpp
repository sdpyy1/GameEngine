#include "hzpch.h"
#include "VulkanComputePass.h"
#include "VulkanShader.h"

namespace Hazel
{
	VulkanComputePass::VulkanComputePass(const ComputePassSpecification& spec) : m_Specification(spec)
	{
		HZ_CORE_VERIFY(spec.Pipeline);

	}

	Ref<Image2D> VulkanComputePass::GetOutput(uint32_t index)
	{
		HZ_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}

	Ref<Image2D> VulkanComputePass::GetDepthOutput()
	{
		HZ_CORE_VERIFY(false, "Not implemented");
		return nullptr;
	}	
	Ref<PipelineCompute> VulkanComputePass::GetPipeline() const
	{
		return m_Specification.Pipeline;
	}

	VulkanComputePass::~VulkanComputePass()
	{

	}

	bool VulkanComputePass::HasDescriptorSets() const
	{
		return m_Specification.Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet().size() > 0;
	}
	const VkDescriptorSet& VulkanComputePass::GetDescriptorSets(uint32_t frameIndex) const
	{
		return m_Specification.Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[frameIndex];
	}
}
