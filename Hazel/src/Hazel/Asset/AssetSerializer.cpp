#include "hzpch.h"
#include "AssetSerializer.h"
#include "Hazel/Asset/Model/Mesh.h"
#include "AssimpMeshImporter.h"

bool Hazel::MeshSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
{
	AssimpMeshImporter importer(metadata.FilePath);
	Ref<MeshSource> meshSource = importer.ImportToMeshSource();
	if (!meshSource)
		return false;

	asset = meshSource;
	asset->Handle = metadata.Handle;
	return true;
}
