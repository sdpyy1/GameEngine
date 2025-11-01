#pragma once

#include "Hazel/Renderer/RenderPass.h"

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
		virtual void SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding) override;
		virtual void SetInput(Ref<StorageBufferSet> texture, uint32_t Binding, bool isInit = false)override;
		virtual void SetInput(Ref<Texture2D> texture, uint32_t Binding)override;
		virtual Ref<Image2D> GetDepthOutput();
		virtual void SetInput(Ref<Image2D> image, uint32_t Binding, bool isInit =false);
		virtual void SetInput(Ref<TextureCube> cubeMap, uint32_t Binding) override;

	private:
		bool IsInvalidated(uint32_t set, uint32_t binding) const;
	private:
		RenderPassSpecification m_Specification;
		std::vector<std::unordered_map<uint32_t, VkImage>> m_BindImages;
	};

}
