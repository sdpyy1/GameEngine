#include "hzpch.h"
#include "MaterialAsset.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/AssetManager.h"
namespace Hazel{
	static const std::string s_AlbedoColorUniform = "u_MaterialUniforms.AlbedoColor";
	static const std::string s_UseNormalMapUniform = "u_MaterialUniforms.UseNormalMap";
	static const std::string s_MetalnessUniform = "u_MaterialUniforms.Metalness";
	static const std::string s_RoughnessUniform = "u_MaterialUniforms.Roughness";
	static const std::string s_EmissionUniform = "u_MaterialUniforms.Emission";
	static const std::string s_TransparencyUniform = "u_MaterialUniforms.Transparency";

	static const std::string s_AlbedoMapUniform = "u_AlbedoTexture";
	static const std::string s_NormalMapUniform = "u_NormalTexture";
	static const std::string s_MetalnessMapUniform = "u_MetalnessTexture";
	static const std::string s_RoughnessMapUniform = "u_RoughnessTexture";


	MaterialAsset::MaterialAsset(bool transparent)
		: m_Transparent(transparent)
	{
		Handle = {};

		if (transparent)
			m_Material = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Transparent"));
		else
			m_Material = Material::Create(Renderer::GetShaderLibrary()->Get("HazelPBR_Static"));

		SetDefaults();
	}

	MaterialAsset::MaterialAsset(Ref<Material> material)
	{
		Handle = {};
		m_Material = Material::Copy(material);
	}
	void MaterialAsset::SetDefaults()
	{
		if (m_Transparent)
		{
			// Set defaults
			SetAlbedoColor(glm::vec3(0.8f));

			// Maps
			ClearAlbedoMap();
		}
		else
		{
			// Set defaults
			SetAlbedoColor(glm::vec3(0.8f));
			SetEmission(0.0f);
			SetUseNormalMap(false);
			SetMetalness(0.0f);
			SetRoughness(0.4f);

			// Maps
			ClearAlbedoMap();
			ClearNormalMap();
			ClearMetalnessMap();
			ClearRoughnessMap();
		}
	}
	void MaterialAsset::SetNormalMap(AssetHandle handle)
	{
		m_Maps.NormalMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_NormalMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearNormalMap();
		}
	}
	void MaterialAsset::SetUseNormalMap(bool value)
	{
		m_Material->Set(s_UseNormalMapUniform, value);
	}
	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_Maps.AlbedoMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_AlbedoMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearAlbedoMap();
		}
	}
	void MaterialAsset::SetRoughnessMap(AssetHandle handle)
	{
		m_Maps.RoughnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_RoughnessMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearRoughnessMap();
		}
	}
	void MaterialAsset::SetMetalnessMap(AssetHandle handle)
	{
		m_Maps.MetalnessMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->Set(s_MetalnessMapUniform, texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearMetalnessMap();
		}
	}
	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		m_Material->Set(s_AlbedoColorUniform, color);

	}
	void MaterialAsset::SetMetalness(float value)
	{
		m_Material->Set(s_MetalnessUniform, value);
	}
	void MaterialAsset::SetRoughness(float value)
	{
		m_Material->Set(s_RoughnessUniform, value);
	}
	void MaterialAsset::SetEmission(float value)
	{
		m_Material->Set(s_EmissionUniform, value);
	}
	void MaterialAsset::ClearAlbedoMap()
	{
		m_Material->Set(s_AlbedoMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearNormalMap()
	{
		m_Material->Set(s_NormalMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearMetalnessMap()
	{
		m_Material->Set(s_MetalnessMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearRoughnessMap()
	{
		m_Material->Set(s_RoughnessMapUniform, Renderer::GetWhiteTexture());

	}
}

