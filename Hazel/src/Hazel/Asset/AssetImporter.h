#pragma once
#include "AssetMetadata.h"
#include "AssetSerializer.h"

namespace Hazel {

	class AssetImporter
	{
	public:
		static void Init();
		static void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset);
		static bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset);
	private:
		static std::unordered_map<AssetType, Scope<AssetSerializer>> s_Serializers;
	};

}
