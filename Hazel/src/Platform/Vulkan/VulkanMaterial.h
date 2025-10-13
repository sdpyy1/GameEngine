#pragma once
#include "VulkanShader.h"
#include "Hazel/Asset/Model/Material.h"
namespace Hazel {
	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const std::string& name = "");
		void UpdateDescriptorSet();
		virtual void SetAlbedoTexture(Ref<Texture2D> texture) override;
		virtual void SetNormalTexture(Ref<Texture2D> texture) override;
		virtual void SetMetalnessTexture(Ref<Texture2D> texture) override;
		virtual void SetRoughnessTexture(Ref<Texture2D> texture) override;
		virtual void Bind() override;
		virtual void RT_Bind() override;

		virtual ~VulkanMaterial() override;
	private:
		Ref<VulkanShader> m_Shader;
		std::string m_Name;
		VkDevice  m_Device;
		std::vector<VkDescriptorSet> m_DescriptorSets; // 材质专属描述符集（按帧分配）
	};
}

