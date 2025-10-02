#include "hzpch.h"
#include "Hazel/Renderer/Shader.h"

#include "Hazel/Renderer/Renderer.h"
#include <Platform/Vulkan/VulkanShader.h>

namespace Hazel {

	Ref_old<Shader> Shader::Create_old(const std::string& filepath)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref_old<Shader> Shader::Create_old(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
	{
		switch (Renderer::Current())
		{
			case RendererAPI::Type::None:    HZ_CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		}

		HZ_CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	Ref<Shader> Shader::Create(const std::string& filepath, bool forceCompile, bool disableOptimization)
	{
		Ref<Shader> result = nullptr;

		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan:result = Ref<VulkanShader>::Create(filepath, forceCompile, disableOptimization);
			break;
		}
		return result;
	}

	void ShaderLibrary::Add_old(const std::string& name, const Ref_old<Shader>& shader)
	{
		HZ_CORE_ASSERT(!Exists_old(name), "Shader already exists!");
		m_Shaders_old[name] = shader;
	}

	void ShaderLibrary::Add_old(const Ref_old<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add_old(name, shader);
	}
	void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader)
	{
		HZ_CORE_ASSERT(!Exists(name), "Shader already exists!");
		m_Shaders[name] = shader;
	}

	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		auto& name = shader->GetName();
		Add(name, shader);
	}
	Ref_old<Shader> ShaderLibrary::Load_old(const std::string& filepath)
	{
		auto shader = Shader::Create_old(filepath);
		Add_old(shader);
		return shader;
	}
	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		Ref<VulkanShader> shader = Shader::Create(filepath);
		Add(shader);
		return shader;
	}
	Ref_old<Shader> ShaderLibrary::Load_old(const std::string& name, const std::string& filepath)
	{
		auto shader = Shader::Create_old(filepath);
		Add_old(name, shader);
		return shader;
	}

	Ref_old<Shader> ShaderLibrary::Get(const std::string& name)
	{
		HZ_CORE_ASSERT(Exists_old(name), "Shader not found!");
		return m_Shaders_old[name];
	}

	bool ShaderLibrary::Exists_old(const std::string& name) const
	{
		return m_Shaders_old.find(name) != m_Shaders_old.end();
	}

	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return false;
	}

}
