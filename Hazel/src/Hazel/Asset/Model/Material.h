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
		virtual void SetEmissionTexture(Ref<Texture2D> texture) = 0;
		virtual void SetAlbedoColor(glm::vec3 color) = 0;
		virtual void SetMetalnessColor(float color) = 0;
		virtual void SetRoughnessColor(float color) = 0;
		virtual void SetEmissionColor(glm::vec3 color) = 0;
		virtual void SetUseNormalTexture(bool value) { bUseNormalTexture = value; }
		virtual ~Material() {}
		struct MaterialPush {
			glm::vec3 AlbedoColor;
			float Metalness;
			float Roughness;
			glm::vec3 Emission;
			uint32_t UseNormalMap;
			// uint32_t padding; // PushRange需要是16字节的整数倍
		};
		MaterialPush BuildPush();
		glm::vec3 m_AlbedoColor = glm::vec3{ 1,1,1 };
		float m_MetalnessColor = 0.0f;
		float m_RoughnessColor = 1.0f;
		glm::vec3& m_EmissionColor = glm::vec3{ 0,0,0 }; 
		bool bUseNormalTexture = false;
	protected:
		Ref<Texture2D> AlbedoTexture = nullptr;
		Ref<Texture2D> NormalTexture = nullptr;
		Ref<Texture2D> MetalnessTexture = nullptr;
		Ref<Texture2D> RoughnessTexture = nullptr;
		Ref<Texture2D> EmissionTexture = nullptr;

	};
}
