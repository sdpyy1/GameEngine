#include "hzpch.h"
#include "Skeleton.h"
#include "Hazel/Math/Math.h"

namespace Hazel
{
	namespace Utils
	{
		glm::mat4 Mat4FromAIMatrix4x4(const aiMatrix4x4& matrix);

	}
	BoneHierarchy::BoneHierarchy(const aiScene* scene) : m_Scene(scene)
	{

	}
	Scope<Skeleton> BoneHierarchy::CreateSkeleton() {
		if (!m_Scene)
		{
			return nullptr;
		}

		ExtractBones();
		if (m_Bones.empty())
		{
			return nullptr;
		}

		auto skeleton = CreateScope<Skeleton>(static_cast<uint32_t>(m_Bones.size()));
		TraverseNode(m_Scene->mRootNode, skeleton.get());

		return skeleton;
	}

	void BoneHierarchy::ExtractBones()
	{
		// Note: ASSIMP does not appear to support import of digital content files that contain _only_ an armature/skeleton and no mesh.
		for (uint32_t meshIndex = 0; meshIndex < m_Scene->mNumMeshes; ++meshIndex)
		{
			const aiMesh* mesh = m_Scene->mMeshes[meshIndex];
			for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex)
			{
				m_Bones.emplace(mesh->mBones[boneIndex]->mName.C_Str());
			}
		}

		// Extract also any nodes that are animated (but don't have any skin bound to them)
		for (uint32_t animationIndex = 0; animationIndex < m_Scene->mNumAnimations; ++animationIndex)
		{
			const aiAnimation* animation = m_Scene->mAnimations[animationIndex];
			for (uint32_t channelIndex = 0; channelIndex < animation->mNumChannels; ++channelIndex)
			{
				const aiNodeAnim* nodeAnim = animation->mChannels[channelIndex];
				m_Bones.emplace(nodeAnim->mNodeName.C_Str());
			}
		}
	}


	void BoneHierarchy::TraverseNode(aiNode* node, Skeleton* skeleton, const glm::mat4& parentTransform)
	{
		if (m_Bones.find(node->mName.C_Str()) != m_Bones.end())
		{
			skeleton->SetTransform(parentTransform);
			aiNode* parent = node->mParent;
			while (parent)
			{
				parent->mTransformation = aiMatrix4x4();
				parent = parent->mParent;
			}
			TraverseBone(node, skeleton, Skeleton::NullIndex);
		}
		else
		{
			auto transform = parentTransform * Utils::Mat4FromAIMatrix4x4(node->mTransformation);
			for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; ++nodeIndex)
			{
				TraverseNode(node->mChildren[nodeIndex], skeleton, transform);
			}
		}
	}


	void BoneHierarchy::TraverseBone(aiNode* node, Skeleton* skeleton, uint32_t parentIndex)
	{
		uint32_t boneIndex = skeleton->AddBone(node->mName.C_Str(), parentIndex, Utils::Mat4FromAIMatrix4x4(node->mTransformation));
		for (uint32_t nodeIndex = 0; nodeIndex < node->mNumChildren; ++nodeIndex)
		{
			if (m_Bones.find(node->mChildren[nodeIndex]->mName.C_Str()) != m_Bones.end())
			{
				TraverseBone(node->mChildren[nodeIndex], skeleton, boneIndex);
			}
			else
			{
				// do not traverse any further.
				// It is not supported to have a non-bone and then more bones below it.
			}
		}
	}
	Skeleton::Skeleton(uint32_t size)
	{
		m_BoneNames.reserve(size);
		m_ParentBoneIndices.reserve(size);
		m_BoneTranslations.reserve(size);
		m_BoneRotations.reserve(size);
		m_BoneScales.reserve(size);
	}
	uint32_t Skeleton::AddBone(std::string name, uint32_t parentIndex, const glm::mat4& transform)
	{
		uint32_t index = static_cast<uint32_t>(m_BoneNames.size());
		m_BoneNames.emplace_back(name);
		m_ParentBoneIndices.emplace_back(parentIndex);
		m_BoneTranslations.emplace_back();
		m_BoneRotations.emplace_back();
		m_BoneScales.emplace_back();
		Math::DecomposeTransform(transform, m_BoneTranslations.back(), m_BoneRotations.back(), m_BoneScales.back());

		return index;
	}
	uint32_t Skeleton::GetBoneIndex(const std::string_view name) const
	{
		for (size_t i = 0; i < m_BoneNames.size(); ++i)
		{
			if (m_BoneNames[i] == name)
			{
				return static_cast<uint32_t>(i);
			}
		}
		return Skeleton::NullIndex;
	}
	std::vector<uint32_t> Skeleton::GetChildBoneIndexes(const uint32_t boneIndex) const
	{
		std::vector<uint32_t> childBoneIndexes;
		for (size_t i = 0; i < m_ParentBoneIndices.size(); ++i)
		{
			if (m_ParentBoneIndices[i] == boneIndex)
			{
				childBoneIndexes.emplace_back(static_cast<uint32_t>(i));
			}
		}
		return childBoneIndexes;
	}
	void Skeleton::SetBones(std::vector<std::string> boneNames, std::vector<uint32_t> parentBoneIndices, std::vector<glm::vec3> boneTranslations, std::vector<glm::quat> boneRotations, std::vector<glm::vec3> boneScales)
	{
		HZ_CORE_ASSERT(parentBoneIndices.size() == boneNames.size());
		HZ_CORE_ASSERT(boneTranslations.size() == boneNames.size());
		HZ_CORE_ASSERT(boneRotations.size() == boneNames.size());
		HZ_CORE_ASSERT(boneScales.size() == boneNames.size());
		m_BoneNames = std::move(boneNames);
		m_ParentBoneIndices = std::move(parentBoneIndices);
		m_BoneTranslations = std::move(boneTranslations);
		m_BoneRotations = std::move(boneRotations);
		m_BoneScales = std::move(boneScales);
	}
}

