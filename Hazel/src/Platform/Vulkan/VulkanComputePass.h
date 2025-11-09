#pragma once
#include "Hazel/Renderer/ComputePass.h"
#include "VulkanShader.h"
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
		virtual void SetInput(Ref<Image2D> texture, uint32_t Binding) override;
		virtual void SetInputOneLayer(Ref<ImageView> imageView, uint32_t Binding, int index) override;
		virtual void SetInput(Ref<Texture2D> texture, uint32_t Binding, InputType type) override;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type) override;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t descriptorIndex) override;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t levelIndex, uint32_t descriptorIndex) override;
		virtual void SetInput(Ref<Image2D> texture, uint32_t Binding, uint32_t descriptorSetIndex) override;

		// v2
		virtual void SetInput(std::string name, Ref<UniformBufferSet> ubSet) override;




		VkDescriptorSet GetMoreDescriptorSet(uint32_t index) { return moreDescriptorSets[index][0]; };
		SetBindingKey GetBinding(std::string name) const;
	private:
		ComputePassSpecification m_Specification;
		std::vector<std::vector<VkDescriptorSet>> moreDescriptorSets;
	};
}
