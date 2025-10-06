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
	private:
		bool IsInvalidated(uint32_t set, uint32_t binding) const;
	private:
		RenderPassSpecification m_Specification;
		DescriptorSetManager m_DescriptorSetManager;
	};

}
