#pragma once
#include "VulkanShader.h"
#include <Hazel/Asset/Model/Material.h>
namespace Hazel {
	class VulkanMaterial : public Material
	{
	public:
		VulkanMaterial(const Ref<Shader>& shader, const std::string& name = "");
		VulkanMaterial(Ref<Material> material, const std::string& name = "");
		virtual ~VulkanMaterial() override;
	private:
		Ref<VulkanShader> m_Shader;
		std::string m_Name;
	};
}

