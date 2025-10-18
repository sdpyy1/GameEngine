#include "hzpch.h"
#include "Mesh.h"
#include "Hazel/Asset/AssimpMeshImporter.h"
namespace Hazel
{
	const Animation* MeshSource::GetAnimation(const std::string& animationName, const Skeleton& skeleton, bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask) const
	{
		// Note: It's possible that the same animation could be requested but with different root motion parameters.
		//       This is pretty edge-case, and not currently supported!
		if (auto it = std::find(m_AnimationNames.begin(), m_AnimationNames.end(), animationName); it != m_AnimationNames.end())
		{
			auto& animation = m_Animations[it - m_AnimationNames.begin()];
			if (!animation)
			{
				// Deferred load of animations.
				// We cannot load them earlier (e.g. in MeshSource constructor) for two reasons:
				// 1) Assimp does not import bones (and hence no skeleton) if the mesh source file contains only animations (and no skin)
				//    This means we need to wait until we know what the skeleton is before we can load the animations.
				// 2) We don't have any way to pass the root motion parameters to the mesh source constructor
				//HZ_CORE_VERIFY(!m_Runtime);
				// 
				//  这里加载缺失的动画
				AssimpMeshImporter importer(m_FilePath);
				importer.ImportAnimation(animationName, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask, animation);
				//auto path = Project::GetEditorAssetManager()->GetFileSystemPath(Handle);
				//AssimpMeshImporter importer(path);
				//importer.ImportAnimation(animationName, skeleton, isMaskedRootMotion, rootTranslationMask, rootRotationMask, animation);
			}
			return animation.get(); // Note: (0x) could still be nullptr (e.g. if import, above, failed.)
		}
		HZ_CORE_ERROR("Animation {0} not found in mesh source {1}!  Please reload the asset.", animationName, m_FilePath);
		return nullptr;
	}


	MeshSource::~MeshSource()
	{
	}
}
