#pragma once
#include <Hazel/Core/UUID.h>
#include "Hazel/Asset/AssetTypes.h"

namespace Hazel {

	using AssetHandle = UUID;

	class Asset
	{
	public:
		AssetHandle Handle = 0;
		uint16_t Flags = (uint16_t)AssetFlag::None; // 正常、缺失、无效

		virtual ~Asset() {}

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual void OnDependencyUpdated(AssetHandle handle) {}

		// 定义UUID相等才是同一个资产
		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	private:
		// If you want to find out whether assets are valid or missing, use AssetManager::IsAssetValid(handle), IsAssetMissing(handle)
		// This cleans up and removes inconsistencies from rest of the code.
		// You simply go AssetManager::GetAsset<Whatever>(handle), and so long as you get a non-null pointer back, you're good to go.
		// No IsValid(), IsFlagSet(AssetFlag::Missing) etc. etc. all throughout the code.
		//friend class EditorAssetManager;
		//friend class RuntimeAssetManager;
		//friend class AssimpMeshImporter;
		//friend class TextureSerializer;

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};
}
