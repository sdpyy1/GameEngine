#pragma once

#include <string>
#include <unordered_map>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Hazel {
	class Shader : public RefCounted
	{
	public:
		virtual ~Shader() = default;
		static Ref<Shader> Create(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath);
		static Ref<Shader> Create(const std::string& name, const std::string& computeFilePath);
		virtual void Reload() = 0;
		virtual void RT_Reload() = 0;
		virtual const std::string& GetName() const = 0;
	};

	class ShaderLibrary : public RefCounted
	{
	public:
		void Add(const std::string& name, const Ref<Shader>& shader);
		void Add(const Ref<Shader>& shader);
		Ref<Shader> LoadCommonShader(const std::string& name, bool isComputeShader = false);
		Ref<Shader> Get(const std::string& name);
	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}
