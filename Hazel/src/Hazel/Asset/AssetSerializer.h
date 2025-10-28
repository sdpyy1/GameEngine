#pragma once
#include "AssetMetadata.h"
namespace Hazel {
	struct AssetSerializationInfo
	{
		uint64_t Offset = 0;
		uint64_t Size = 0;
	};

	class AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const = 0;
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const = 0;
	};
	class MeshSourceSerializer : public AssetSerializer
	{
	public:
		virtual void Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const override { HZ_CORE_INFO("目前还不支持序列化Mesh"); }
		virtual bool TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const override;
	};
}


