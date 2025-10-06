#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Hazel {

	class Shader : public RefCounted
	{
	public:
		// TODO: ��ʱ��ֻ����Vulkan�����Ҫ���󣬸����ͣ���Vulkan�ڲ���switch����
		struct DescriptorBinding {
			uint32_t binding;               // �󶨵�����
			VkDescriptorType type;          // ��Դ���ͣ�UBO���������ȣ�
			VkShaderStageFlags stageFlags;  // �ɼ�����ɫ���׶�
			uint32_t count = 1;             // ���鳤�ȣ�Ĭ��1����������Դ��
		};
		struct ShaderSpecification {
			std::vector<DescriptorBinding> bindings;
		};
		virtual ~Shader() = default;
		static Ref<Shader> Create(const std::string& name,const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec);
		virtual void Reload() = 0;
		virtual void RT_Reload() = 0;
		virtual const std::string& GetName() const = 0;

	};

	class ShaderLibrary : public RefCounted
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> LoadCommonShader(const std::string& name, const std::string& vertFileShader, const std::string& fragFileShader,Shader::ShaderSpecification spec);
		Ref<Shader> Get(const std::string& name);
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};

}
