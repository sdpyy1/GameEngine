#include "hzpch.h"
#include "Material.h"
#include "Platform/Vulkan/VulkanMaterial.h"

#include "Hazel/Renderer/RendererAPI.h"

namespace Hazel {

	Ref<Material> Material::Create(const Ref<Shader>& shader, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None : return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanMaterial>::Create(shader, name);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Material> Material::Copy(const Ref<Material>& other, const std::string& name)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanMaterial>::Create(other, name);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
