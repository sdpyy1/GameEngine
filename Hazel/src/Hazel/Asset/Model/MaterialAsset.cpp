#include "hzpch.h"
#include "MaterialAsset.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/AssetManager.h"
namespace Hazel{
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
		m_Material = material;
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
			m_Material->SetNormalTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearNormalMap();
		}
	}
	void MaterialAsset::SetUseNormalMap(bool value)
	{
		//m_Material->Set(s_UseNormalMapUniform, value);
	}
	void MaterialAsset::SetAlbedoMap(AssetHandle handle)
	{
		m_Maps.AlbedoMap = handle;

		if (handle)
		{
			Ref<Texture2D> texture = AssetManager::GetAsset<Texture2D>(handle);
			m_Material->SetAlbedoTexture(texture);
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
			m_Material->SetRoughnessTexture(texture);
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
			m_Material->SetMetalnessTexture(texture);
			AssetManager::RegisterDependency(handle, Handle);
		}
		else
		{
			ClearMetalnessMap();
		}
	}
	void MaterialAsset::SetAlbedoColor(const glm::vec3& color)
	{
		//m_Material->Set(s_AlbedoColorUniform, color);

	}
	void MaterialAsset::SetMetalness(float value)
	{
		//m_Material->Set(s_MetalnessUniform, value);
	}
	void MaterialAsset::SetRoughness(float value)
	{
		//m_Material->Set(s_RoughnessUniform, value);
	}
	void MaterialAsset::SetEmission(float value)
	{
		//m_Material->Set(s_EmissionUniform, value);
	}
	void MaterialAsset::ClearAlbedoMap()
	{
		//m_Material->Set(s_AlbedoMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearNormalMap()
	{
		//m_Material->Set(s_NormalMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearMetalnessMap()
	{
		//m_Material->Set(s_MetalnessMapUniform, Renderer::GetWhiteTexture());

	}
	void MaterialAsset::ClearRoughnessMap()
	{
		//m_Material->Set(s_RoughnessMapUniform, Renderer::GetWhiteTexture());

	}
}

