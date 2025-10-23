#include "hzpch.h"
#include "AssetManager.h"
namespace Hazel {
	std::unordered_map<AssetHandle, Ref<Asset>> AssetManager::m_MemoryAssets;
	std::unordered_map<AssetHandle, std::unordered_set<AssetHandle>>AssetManager::m_AssetDependencies;
	 std::map<std::filesystem::path, Ref<Asset>>AssetManager::MesheSourceCacheMap;

}
