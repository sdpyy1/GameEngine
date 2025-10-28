#include "hzpch.h"
#include "AssetImporter.h"
namespace Hazel {
	std::unordered_map<AssetType, Scope<AssetSerializer>> AssetImporter::s_Serializers;

	void AssetImporter::Init()
	{
		s_Serializers.clear();
		s_Serializers[AssetType::MeshSource] = CreateScope<MeshSourceSerializer>();
	}

	void AssetImporter::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			HZ_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return;
		}

		s_Serializers[asset->GetAssetType()]->Serialize(metadata, asset);
	}


	bool AssetImporter::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset)
	{
		if (s_Serializers.find(metadata.Type) == s_Serializers.end())
		{
			HZ_CORE_WARN("There's currently no importer for assets of type {0}", metadata.FilePath.stem().string());
			return false;
		}

		 HZ_CORE_TRACE("AssetImporter::TryLoadData - {}", metadata.FilePath);
		return s_Serializers[metadata.Type]->TryLoadData(metadata, asset);
	}
}
