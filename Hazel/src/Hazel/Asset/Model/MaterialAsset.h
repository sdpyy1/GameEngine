#pragma once

#include "Hazel/Asset/Asset.h"

#include <map>
#include "Material.h"

namespace Hazel {

	// 存储着各个贴图的UUID
	class MaterialAsset : public Asset
	{
	public:
		explicit MaterialAsset(bool transparent = false);
		explicit MaterialAsset(Ref<Material> material);
		void SetDefaults();
		void SetNormalMap(AssetHandle handle);
		void SetUseNormalMap(bool value);
		void SetAlbedoMap(AssetHandle handle);
		void SetRoughnessMap(AssetHandle handle);
		void SetMetalnessMap(AssetHandle handle);
		void SetAlbedoColor(const glm::vec3& color);
		void SetMetalness(float value);
		void SetRoughness(float value);
		void SetEmission(float value);

		void ClearAlbedoMap();
		void ClearNormalMap();
		void ClearMetalnessMap();
		void ClearRoughnessMap();

	private:
		Ref<Material> m_Material;

		struct MapAssets
		{
			AssetHandle AlbedoMap = 0;
			AssetHandle NormalMap = 0;
			AssetHandle MetalnessMap = 0;
			AssetHandle RoughnessMap = 0;
		} m_Maps;

		bool m_Transparent = false;

	
	};

	class MaterialTable : public RefCounted
	{
	
	};

}
