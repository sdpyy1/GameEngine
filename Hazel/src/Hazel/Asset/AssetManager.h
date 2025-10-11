#pragma once
#include "Asset.h"
namespace Hazel {
	class AssetManager {
	public:
		template<typename TAsset>
		static AssetHandle AddMemoryOnlyAsset(Ref<TAsset> asset) {
			static_assert(std::is_base_of<Asset, TAsset>::value, "AddMemoryOnlyAsset only works for types derived from Asset");
			asset->Handle = AssetHandle(); // ����UUID
			m_MemoryAssets[asset->Handle] = asset; // ����
			return asset->Handle;
		};
		template<typename T>
		static Ref<T> GetAsset(AssetHandle assetHandle)
		{
			Ref<Asset> asset = m_MemoryAssets[assetHandle];
			if (!asset->IsValid()) return nullptr;
			return asset.As<T>();
		}

		static void RegisterDependency(AssetHandle handle, AssetHandle dependency) { 
			// TODO:����ȱ��handleУ��
			m_AssetDependencies[handle].insert(dependency);
		}

	private:		
		static std::unordered_map<AssetHandle, Ref<Asset>> m_MemoryAssets; // ���ص�Asset
		static std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>> m_AssetDependencies;

	};
}
