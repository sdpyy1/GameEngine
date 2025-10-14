#pragma once
#include <Hazel/Renderer/Shader.h>
#include <Hazel/Renderer/Renderer.h>
#include <Hazel/Renderer/Texture.h>
namespace Hazel {
	class Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");

		virtual void SetAlbedoTexture(Ref<Texture2D> texture) = 0;
		virtual void SetNormalTexture(Ref<Texture2D> texture) = 0;
		virtual void SetMetalnessTexture(Ref<Texture2D> texture) = 0;
		virtual void SetRoughnessTexture(Ref<Texture2D> texture) = 0;
		virtual ~Material() {}
	protected:
		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalTexture = nullptr;
		Ref<Texture2D> MetalnessTexture = nullptr;
		Ref<Texture2D> RoughnessTexture = nullptr;
	};

}
