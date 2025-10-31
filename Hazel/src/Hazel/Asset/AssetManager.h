#pragma once
#include "Asset.h"
#include "Hazel/Asset/Model/MaterialAsset.h"
#include "AssetMetadata.h"
#include "AssetImporter.h"
namespace Hazel {
	class AssetManager {
	public:
		template<typename TAsset>
		static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset) {
			static_assert(std::is_base_of<Asset, TAsset>::value, "AddMemoryOnlyAsset only works for types derived from Asset");
			asset->Handle = AssetHandle(); // 生成UUID
			m_MemoryAssets[asset->Handle] = asset; // 缓存
			return asset->Handle;
		};
		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			Ref<Asset> asset = m_MemoryAssets[assetHandle];
			if (asset == nullptr || !asset->IsValid()) return nullptr;
			return asset.As<T>();
		}
		// 加载Mesh并缓存
		static Ref<Asset> GetMesh(std::filesystem::path path) {
			Ref<Asset> meshAsset = MesheSourceCacheMap[path];
			if (!meshAsset) {
				AssetMetadata metadata;
				metadata.FilePath = path;
				metadata.Type = AssetType::MeshSource;
				AssetImporter::TryLoadData(metadata, meshAsset);
			}
			MesheSourceCacheMap[path] = meshAsset;
			m_MemoryAssets[meshAsset->Handle] = meshAsset;
			return meshAsset;
		}
		static void RegisterDependency(AssetHandle handle, AssetHandle dependency) {
			// TODO:这里缺少handle校验
			m_AssetDependencies[handle].insert(dependency);
		}

	private:
		static std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets;
		static std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependencies;
		static std::map<std::filesystem::path, Ref<Asset>> MesheSourceCacheMap;
	};
}
