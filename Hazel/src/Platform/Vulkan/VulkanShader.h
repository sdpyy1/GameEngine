#pragma once
#include"Hazel/Renderer/Shader.h"
#include <vulkan/vulkan.h>
namespace Hazel {

	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec);

		void Reload() override;
		void RT_Reload() override;
		void createDescriptorSetLayout();
		void createDescriptorSet();
		VkShaderModule GetVertShaderModule() { return m_VertShaderModule; }
		VkShaderModule GetFragShaderModule() { return m_FragShaderModule; }
		VkDescriptorSetLayout* GetDescriptorSetLayout() { return &m_DescriptorSetLayout; }
		std::vector<VkDescriptorSet> GetDescriptorSet() { return m_DescriptorSets; }
		virtual ~VulkanShader();
		void Release();
		virtual const std::string& GetName() const override { return m_Name; }
		//const std::vector<ShaderResource::ShaderDescriptorSet>& GetShaderDescriptorSets() const { return m_ReflectionData.ShaderDescriptorSets; }

	private:
		std::string m_Name;
		std::string m_VertFilePath;
		std::string m_FragFilePath;
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
		ShaderSpecification m_Spec;
		VkDescriptorSetLayout m_DescriptorSetLayout;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;
	};
}
