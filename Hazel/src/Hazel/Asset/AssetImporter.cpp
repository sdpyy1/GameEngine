#include "hzpch.h"
#include "AssetImporter.h"
namespace Hazel {

	void AssetImporter::Init()
	{
		// 每种资源的导入都是通过序列化器
		s_Serializers.clear();
		//s_Serializers[AssetType::Prefab] = CreateScope<PrefabSerializer>();
		//s_Serializers[AssetType::Texture] = CreateScope<TextureSerializer>();
		//s_Serializers[AssetType::Mesh] = CreateScope<MeshSerializer>();
		//s_Serializers[AssetType::StaticMesh] = CreateScope<StaticMeshSerializer>();
		s_Serializers[AssetType::MeshSource] = CreateScope<MeshSourceSerializer>();
		//s_Serializers[AssetType::Material] = CreateScope<MaterialAssetSerializer>();
		//s_Serializers[AssetType::EnvMap] = CreateScope<EnvironmentSerializer>();
		//s_Serializers[AssetType::Audio] = CreateScope<AudioFileSourceSerializer>();
		//s_Serializers[AssetType::SoundConfig] = CreateScope<SoundConfigSerializer>();
		//s_Serializers[AssetType::Scene] = CreateScope<SceneAssetSerializer>();
		//s_Serializers[AssetType::Font] = CreateScope<FontSerializer>();
		//s_Serializers[AssetType::MeshCollider] = CreateScope<MeshColliderSerializer>();
		//s_Serializers[AssetType::SoundGraphSound] = CreateScope<SoundGraphGraphSerializer>();
		//s_Serializers[AssetType::Skeleton] = CreateScope<SkeletonAssetSerializer>();
		//s_Serializers[AssetType::Animation] = CreateScope<AnimationAssetSerializer>();
		//s_Serializers[AssetType::AnimationGraph] = CreateScope<AnimationGraphAssetSerializer>();
		//s_Serializers[AssetType::ScriptFile] = CreateScope<ScriptFileSerializer>();
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
