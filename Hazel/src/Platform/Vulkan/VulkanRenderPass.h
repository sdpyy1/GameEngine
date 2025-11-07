#pragma once

#include "Hazel/Renderer/RenderPass.h"

#include "vulkan/vulkan.h"
#include "VulkanShader.h"

namespace Hazel {
	class VulkanRenderPass : public RenderPass
	{
	public:
		VulkanRenderPass(const RenderPassSpecification& spec);
		virtual ~VulkanRenderPass();

		virtual RenderPassSpecification& GetSpecification() override { return m_Specification; }
		virtual const RenderPassSpecification& GetSpecification() const override { return m_Specification; }
		virtual Ref<Pipeline> GetPipeline() const override { return m_Specification.Pipeline; };

		virtual Ref<Image2D> GetDepthOutput();

		// 新版,自动获取set binding
		virtual void SetInput(std::string name, Ref<UniformBufferSet> UboSet) override;
		virtual void SetInput(std::string name, Ref<Texture2D> texture,bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<Image2D> image, bool isInit = false) override;
		virtual void SetInput(std::string name, Ref<StorageBufferSet> SBSet) override;
		virtual void SetInput(std::string name, Ref<TextureCube> cubeMap,bool isInit = false) override;

	private:
		SetBindingKey GetBinding(std::string name) const;
		Ref<VulkanShader> GetVulkanShader() { return GetSpecification().Pipeline->GetShader().As<VulkanShader>(); }
	private:
		RenderPassSpecification m_Specification;
		std::vector<std::unordered_map<SetBindingKey, VkImage, SetBindingKeyHash>> m_BindImages; // 每个飞行帧的每个binding位置的图片缓存，为了避免重复Update
	};

}
