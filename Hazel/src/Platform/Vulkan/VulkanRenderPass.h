#pragma once

#include "Hazel/Renderer/RenderPass.h"

#include "DescriptorSetManager.h"

#include "vulkan/vulkan.h"

namespace Hazel {


	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
		virtual Ref<Pipeline> GetPipeline() const override { return m_Specification.Pipeline; };
		virtual void SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding);
		virtual void SetInput(Ref<Texture2D> texture, uint32_t Binding);

	private:
		bool IsInvalidated(uint32_t set, uint32_t binding) const;
	private:
		RenderPassSpecification m_Specification;
		DescriptorSetManager m_DescriptorSetManager;
	};

}
