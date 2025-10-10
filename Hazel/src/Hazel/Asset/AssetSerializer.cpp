#include "hzpch.h"
#include "AssetSerializer.h"
#include "Hazel/scene/Model/Mesh.h"
#include "AssimpMeshImporter.h"

bool Hazel::MeshSourceSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
{
	AssimpMeshImporter importer(Project::GetEditorAssetManager()->GetFileSystemPathString(metadata));
	Ref<MeshSource> meshSource = importer.ImportToMeshSource();
	if (!meshSource)
		return false;

	asset = meshSource;
	asset->Handle = metadata.Handle;
	return true;
}
