#include "hzpch.h"
#include "Hazel/Renderer/Shader.h"

#include "Hazel/Renderer/Renderer.h"
#include <Platform/Vulkan/VulkanShader.h>

namespace Hazel {


	Ref<Shader> Shader::Create(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec)
	{
		Ref<Shader> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan:result = Ref<VulkanShader>::Create(name, vertFilePath, fragFilePath, spec);
			break;
		}
		return result;
	}

	
	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) == m_Shaders.end());
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}
	
	Ref<Shader> ShaderLibrary::LoadCommonShader(const std::string& name, const std::string& vertFileShader, const std::string& fragFileShader,Shader::ShaderSpecification spec)
	{
		Ref<VulkanShader> shader = Shader::Create(name, vertFileShader, fragFileShader,spec);
		Add(shader);
		return shader;
	}


	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(m_Shaders.find(name) != m_Shaders.end());
		return m_Shaders[name];
	}

}
