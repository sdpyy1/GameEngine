#include "hzpch.h"
#include "Hazel/Renderer/Texture.h"

#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Platform/Vulkan/VulkanTexture.h"

namespace Hazel {
	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanTexture2D>::Create(specification);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, const std::filesystem::path& filepath)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanTexture2D>::Create(specification, filepath);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanTexture2D>::Create(specification, imageData);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}


	Ref<TextureCube> TextureCube::Create(const TextureSpecification& specification, Buffer imageData)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		case RendererAPI::Type::Vulkan: return Ref<VulkanTextureCube>::Create(specification, imageData);
		}
		ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
}
