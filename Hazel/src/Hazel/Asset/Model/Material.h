#pragma once
#include <Hazel/Renderer/Shader.h>
#include <Hazel/Renderer/Texture.h>
namespace Hazel {

	enum class MaterialFlag
	{
		None = BIT(0),
		DepthTest = BIT(1),
		Blend = BIT(2),
		TwoSided = BIT(3),
		DisableShadowCasting = BIT(4)
	};

	class Material : public RefCounted
	{
	public:
		static Ref<Material> Create(const Ref<Shader>& shader, const std::string& name = "");
		static Ref<Material> Copy(const Ref<Material>& other, const std::string& name = "");

		virtual void Set(const std::string& name, const glm::vec3& value) = 0;
		virtual void Set(const std::string& name, float value) = 0;

		virtual ~Material() {}
	};

}
