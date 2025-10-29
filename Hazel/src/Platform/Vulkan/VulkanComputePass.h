#pragma once
#include "Hazel/Renderer/ComputePass.h"
namespace Hazel
{
	class VulkanComputePass : public ComputePass
	{
	public:
		VulkanComputePass(const ComputePassSpecification& spec);
		virtual ~VulkanComputePass();
		ComputePassSpecification& GetSpecification() override { return m_Specification; }
		const ComputePassSpecification& GetSpecification() const override { return m_Specification; }
		Ref<Shader> GetShader() const override { return m_Specification.Pipeline->GetShader(); }
		Ref<Image2D> GetOutput(uint32_t index) override;
		Ref<Image2D> GetDepthOutput() override;
		Ref<PipelineCompute> GetPipeline() const override;
		bool HasDescriptorSets() const override;
		const VkDescriptorSet& GetDescriptorSets(uint32_t frameIndex) const;
	private:
		ComputePassSpecification m_Specification;
	};
}
