#pragma once
#include"Hazel/Renderer/Shader.h"
#include <vulkan/vulkan.h>
namespace Hazel {
	struct DescriptorBinding {
		uint32_t binding;               // 绑定点索引
		VkDescriptorType type;          // 资源类型（UBO、采样器等）
		VkShaderStageFlags stageFlags;  // 可见的着色器阶段
		uint32_t count = 1;             // 数组长度（默认1，非数组资源）
		uint32_t set;
	};
	struct PushConstantRange {
		VkShaderStageFlags shaderStage;  // 作用的着色器阶段（顶点、片段等）
		uint32_t offset = 0;             // 数据起始偏移量（字节）
		uint32_t size = 0;               // 数据总大小（字节）
	};
	struct ShaderSpecification {
		std::vector<DescriptorBinding> bindings;
		std::vector<PushConstantRange> pushConstantRanges;
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
		std::vector<VkDescriptorSet> GetDescriptorSet() { return m_DescriptorSets; }
		VkDescriptorPool GetDescriptorPool() { return m_DescriptorPool; }
		virtual ~VulkanShader();
		void Release();
		const std::vector<PushConstantRange>& GetPushConstantRanges() const { return m_Spec.pushConstantRanges; }

		virtual const std::string& GetName() const override { return m_Name; }
		void ReflectSPIRVAndPopulateSpec(const std::vector<char>& spirvCode, VkShaderStageFlagBits stage);
	private:
		std::string m_Name;
		std::string m_VertFilePath;
		std::string m_FragFilePath;
		std::string m_ComputePath;
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
