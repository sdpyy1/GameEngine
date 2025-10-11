#pragma once
#include <Hazel/Asset/Model/Mesh.h>
namespace Hazel {
	class AssimpMeshImporter
	{
	public:
		AssimpMeshImporter(const std::filesystem::path& path);

		Ref<MeshSource> ImportToMeshSource();
	private:
		void TraverseNodes(Ref<MeshSource> meshSource, void* assimpNode, uint32_t nodeIndex, const glm::mat4& parentTransform = glm::mat4(1.0f), uint32_t level = 0);
	private:
		const std::filesystem::path m_Path;
	};
}
