#pragma once
#include "Model/Skeleton.h"
#include <assimp/scene.h>
#include "Hazel/Asset/Model/Animation.h"
namespace Hazel {
	class AssimpAnimationImporter
	{
	public:
		static Scope<Skeleton> ImportSkeleton(const aiScene* scene);
		static std::vector<std::string> GetAnimationNames(const aiScene* scene);
		static Scope<Animation> ImportAnimation(const aiScene* scene, const uint32_t animationIndex, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask);
		static uint32_t GetAnimationIndex(const aiScene* scene, std::string_view animationName);

	};
}

