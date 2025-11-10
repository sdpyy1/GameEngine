#pragma once
#include"Hazel/Renderer/Shader.h"
#include <vulkan/vulkan.h>
namespace Hazel {
	struct DescriptorBinding {
		uint32_t binding; 
		VkDescriptorType type; 
		VkShaderStageFlags stageFlags; 
		uint32_t count = 1;  
		uint32_t set;
	};
	struct PushConstantRange {
		VkShaderStageFlags shaderStage; 
		uint32_t offset = 0; 
		uint32_t size = 0; 
	};
	struct ShaderSpecification {
		std::vector<DescriptorBinding> bindings;
		std::vector<PushConstantRange> pushConstantRanges;
	};
	struct SetBindingKey {
		uint32_t set;
		uint32_t binding;
		bool operator==(SetBindingKey const& o) const { return set == o.set && binding == o.binding; }
	};
	struct SetBindingKeyHash {
		std::size_t operator()(SetBindingKey const& k) const noexcept {
			std::size_t h1 = std::hash<uint32_t>{}(k.set);
			std::size_t h2 = std::hash<uint32_t>{}(k.binding);
			return h1 ^ (h2 << 1);
		}
	};
	class VulkanShader : public Shader
	{
	public:
		VulkanShader() = default;
		VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath);
		VulkanShader(const std::string& name, const std::string& computeFilePath);

		void Reload() override;
		void RT_Reload() override;
		void createDescriptorSetLayout();
		void createDescriptorPool(const std::unordered_map<uint32_t, uint32_t>& setMaxCounts);
		void createDescriptorSet(); // legency：只创建set=0的资源描述符集，作为初始化时默认创建
		std::vector<VkDescriptorSet> createDescriptorSet(uint32_t targetSet);

		VkShaderModule GetVertShaderModule() { return m_VertShaderModule; }
		VkShaderModule GetFragShaderModule() { return m_FragShaderModule; }
		VkShaderModule GetComputeShaderModule() { return m_ComputeShaderModule; }
		const std::vector<VkDescriptorSetLayout>& GetDescriptorSetLayout() const { return m_DescriptorSetLayouts; }
		std::vector<VkDescriptorSet> GetDescriptorSet(uint32_t set) { return m_DescriptorSets; }
		std::vector<VkDescriptorSet> GetDescriptorSet() { return m_DescriptorSets; }
		VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }
		virtual ~VulkanShader();
		const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_Spec.pushConstantRanges; }
		SetBindingKey VulkanShader::getSetAndBinding(const std::string& name);
		virtual const std::string& GetName() const override { return m_Name; }
		void ReflectSPIRVAndPopulateSpec(const std::vector<char>& spirvCode, VkShaderStageFlagBits stage);
		std::string GetVertFilePath() const { return m_VertFilePath; }
        std::string GetFragFilePath() const { return m_FragFilePath; }
        std::string GetComputeFilePath() const { return m_ComputePath; }
		VkDescriptorType GetDescriptorType(const std::string& name){return m_NameToDescriptorDel[name].type;}
		void Release();
	private:

		std::string m_Name;
		std::string m_VertFilePath;
		std::string m_FragFilePath;
		std::string m_ComputePath = "";
		std::unordered_map<std::string, SetBindingKey> m_NameToBinding;
		std::unordered_map<std::string, DescriptorBinding> m_NameToDescriptorDel;
		bool m_IsCompute = false;
		VkShaderModule m_VertShaderModule;
		VkShaderModule m_FragShaderModule;
		VkShaderModule m_ComputeShaderModule;
		ShaderSpecification m_Spec;
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts;
		VkDescriptorPool m_DescriptorPool;
		std::vector<VkDescriptorSet> m_DescriptorSets;	
	};
}
