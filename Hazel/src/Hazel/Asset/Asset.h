#pragma once
#include <Hazel/Core/UUID.h>
#include "Hazel/Asset/AssetTypes.h"

namespace Hazel {

	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle = 0;
		uint16_t Flags = (uint16_t)AssetFlag::None; 

		virtual ~Asset() = default;

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual void OnDependencyUpdated(AssetHandle handle) {}

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}

		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	private:
		friend class AssimpMeshImporter;
		friend class AssetManager;
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
