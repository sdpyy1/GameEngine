#include "hzpch.h"
#include "Hazel/Renderer/Texture.h"

#include "Hazel/Renderer/Renderer.h"
#include "Platform/OpenGL/OpenGLTexture.h"
#include "Platform/Vulkan/VulkanTexture.h"

namespace Hazel {
	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
			//case RendererAPI::APIType::Vulkan: return Ref_old<VulkanTexture2D>::Create(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, const std::filesystem::path& filepath)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
			//case RendererAPI::APIType::Vulkan: return Ref_old<VulkanTexture2D>::Create(specification, filepath);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer1 imageData)
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::Type::None: return nullptr;
			case RendererAPI::Type::Vulkan: return Ref<VulkanTexture2D>::Create(specification, imageData);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	Ref_old<Texture2D> Texture2D::Create_old(const TextureSpecification& specification)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		//case RendererAPI::APIType::Vulkan: return Ref_old<VulkanTexture2D>::Create(specification);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref_old<Texture2D> Texture2D::Create_old(const TextureSpecification& specification, const std::filesystem::path& filepath)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		//case RendererAPI::APIType::Vulkan: return Ref_old<VulkanTexture2D>::Create(specification, filepath);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref_old<Texture2D> Texture2D::Create_old(const TextureSpecification& specification, Buffer1 imageData)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		//case RendererAPI::APIType::Vulkan: return CreateRef<Texture>();
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

	Ref_old<TextureCube> TextureCube::Create_old(const TextureSpecification& specification, Buffer1 imageData)
	{
		switch (RendererAPI::Current())
		{
		case RendererAPI::Type::None: return nullptr;
		//case RendererAPI::APIType::Vulkan: return Ref_old<VulkanTextureCube>::Create(specification, imageData);
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}

}
