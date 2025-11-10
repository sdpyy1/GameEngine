#pragma once
#include "Hazel/Renderer/ComputePass.h"
#include "VulkanShader.h"
#include "DescriptorManager.h"
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
		virtual void SetInputOneLayer(Ref<ImageView> imageView, uint32_t Binding, int index) override;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t descriptorIndex) override;
		virtual void SetInput(Ref<TextureCube> texture, uint32_t Binding, InputType type, uint32_t levelIndex, uint32_t descriptorIndex) override;
		virtual void SetInput(Ref<Image2D> texture, uint32_t Binding, uint32_t descriptorSetIndex) override;

		// v2
		virtual void SetInput(std::string name, Ref<UniformBufferSet> UboSet) override;
		virtual void SetInput(std::string name, Ref<Texture2D> texture, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<StorageBufferSet> SBSet) override;
		virtual void SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<Image2D> image, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<ImageView> image) override;




		VkDescriptorSet GetMoreDescriptorSet(uint32_t index) { return moreDescriptorSets[index][0]; };
		SetBindingKey GetBinding(std::string name) const;
	private:
		ComputePassSpecification m_Specification;
		DescriptorManager m_DescriptorManager;
		std::vector<std::vector<VkDescriptorSet>> moreDescriptorSets;
	};
}
