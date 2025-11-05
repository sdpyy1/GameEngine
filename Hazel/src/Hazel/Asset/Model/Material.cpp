#include "hzpch.h"
#include "Hazel/Asset/Model/Material.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {
	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	MaterialPush Material::BuildPush()
	{
		MaterialPush push;
		push.AlbedoColor = m_AlbedoColor;
		push.Emission = m_EmissionColor;
        push.Metalness = m_MetalnessColor;
        push.Roughness = m_RoughnessColor;
		push.UseNormalMap = bUseNormalTexture ? 1u : 0u;
		return push;
	}

}
