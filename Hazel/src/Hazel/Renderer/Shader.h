#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Hazel {

	class Shader : public RefCounted
	{
	public:
		// TODO: 临时，只满足Vulkan，如果要抽象，改类型，在Vulkan内部再switch即可
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
		virtual ~Shader() = default;
		static Ref<Shader> Create(const std::string& name,const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec);
		static Ref<Shader> Create(const std::string& name,const std::string& computeFilePath, ShaderSpecification spec);
		virtual void Reload() = 0;
		virtual void RT_Reload() = 0;
		virtual const std::string& GetName() const = 0;
	};

	class ShaderLibrary : public RefCounted
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> LoadCommonShader(const std::string& name,Shader::ShaderSpecification spec, bool isComputeShader = false);
		Ref<Shader> Get(const std::string& name);
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}
