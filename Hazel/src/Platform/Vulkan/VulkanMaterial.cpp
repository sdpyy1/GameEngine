#include "hzpch.h"
#include "VulkanMaterial.h"
#include <Hazel/Renderer/Shader.h>

namespace Hazel {
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader.As<VulkanShader>()), m_Name(name)
	{
		
	}
	VulkanMaterial::VulkanMaterial(Ref<Material> material, const std::string& name)
	{
	}
	VulkanMaterial::~VulkanMaterial()
	{
	}
}
