#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetManager.h"
#include <Hazel/Math/AABB.h>
#include <Hazel/Renderer/VertexBuffer.h>
#include <Hazel/Renderer/IndexBuffer.h>
#include "Hazel/Asset/Model/Material.h"
#include "Hazel/Asset/Model/MaterialAsset.h"
#include "Skeleton.h"
#include "Animation.h"

namespace Hazel {
	struct Vertex {
		glm::vec3 Position;  // ���з���Ϊ 0
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
		// ���ڴ���pipeline
		static VertexBufferLayout GetVertexLayout() {
			VertexBufferElement Position(ShaderDataType::Float3, "Position");
			VertexBufferElement Normal(ShaderDataType::Float3, "Normal");
			VertexBufferElement Tangent(ShaderDataType::Float3, "Tangent");
			VertexBufferElement Binormal(ShaderDataType::Float3, "Binormal");
			VertexBufferElement Texcoord(ShaderDataType::Float2, "Texcoord");
			return { Position ,Normal ,Tangent ,Binormal,Texcoord };
		}
	};
	struct Index
	{
		uint32_t V1, V2, V3;
	};
	struct Triangle
	{
		Vertex V0, V1, V2;

		Triangle(const Vertex& v0, const Vertex& v1, const Vertex& v2)
			: V0(v0), V1(v1), V2(v2) {
		}
	};
	struct BoneInfo
	{
		glm::mat4 InverseBindPose;
		uint32_t BoneIndex;
		BoneInfo(const glm::mat4& inverseBindPose, uint32_t boneIndex)
			: InverseBindPose(inverseBindPose), BoneIndex(boneIndex) {
		}
	};
	struct BoneInfluence
	{
		uint32_t BoneInfoIndices[4] = { 0, 0, 0, 0 };  // Ӱ�쵱ǰ����Ĺ������������4����
		float Weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f }; // ��Ӧ������Ӱ��Ȩ�أ����4����

		void AddBoneData(uint32_t boneInfoIndex, float weight)
		{
			if (weight < 0.0f || weight > 1.0f)
			{
				HZ_CORE_WARN("Vertex bone weight is out of range. We will clamp it to [0, 1] (BoneID={0}, Weight={1})", boneInfoIndex, weight);
				weight = std::clamp(weight, 0.0f, 1.0f);
			}
			if (weight > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					if (Weights[i] == 0.0f)
					{
						BoneInfoIndices[i] = boneInfoIndex;
						Weights[i] = weight;
						return;
					}
				}

				// Note: when importing from assimp we are passing aiProcess_LimitBoneWeights which automatically keeps only the top N (where N defaults to 4)
				//       bone weights (and normalizes the sum to 1), which is exactly what we want.
				//       So, we should never get here.
				HZ_CORE_WARN("Vertex has more than four bones affecting it, extra bone influences will be discarded (BoneID={0}, Weight={1})", boneInfoIndex, weight);
			}
		}

		void NormalizeWeights()
		{
			float sumWeights = 0.0f;
			for (size_t i = 0; i < 4; i++)
			{
				sumWeights += Weights[i];
			}
			if (sumWeights > 0.0f)
			{
				for (size_t i = 0; i < 4; i++)
				{
					Weights[i] /= sumWeights;
				}
			}
		}
	};
	struct MeshNode
	{
		uint32_t Parent = 0xffffffff;
		std::vector<uint32_t> Children;
		std::vector<uint32_t> Submeshes;

		std::string Name;
		glm::mat4 LocalTransform;

		inline bool IsRoot() const { return Parent == 0xffffffff; }
	};
	class Submesh
	{
	public:
		uint32_t BaseVertex;
		uint32_t BaseIndex;
		uint32_t MaterialIndex;
		uint32_t IndexCount;
		uint32_t VertexCount;

		glm::mat4 Transform{ 1.0f }; // World transform
		glm::mat4 LocalTransform{ 1.0f }; // ������Լ��ĸ����ı任
		AABB BoundingBox;

		std::string NodeName, MeshName;
		bool IsRigged = false;
	};
	class MeshSource : public Asset {
	public:
		MeshSource() = default;
		Ref<VertexBuffer> GetVertexBuffer() {return m_VertexBuffer;}
		Ref<IndexBuffer> GetIndexBuffer() {return m_IndexBuffer;}
		std::vector<Submesh>& GetSubmeshes() { return m_Submeshes; }
		AssetHandle GetMaterialHandle(uint32_t index) {return m_Materials[index];}
		bool HasSkeleton() const { return (bool)m_Skeleton; }
		const Skeleton* GetSkeleton() const { return m_Skeleton.get(); }
		const Animation* GetAnimation(const std::string& animationName, const Skeleton& skeleton, const bool isMaskedRootMotion, const glm::vec3& rootTranslationMask, float rootRotationMask) const;
		bool IsSubmeshRigged(uint32_t submeshIndex) const { return m_Submeshes[submeshIndex].IsRigged; }
		Ref<VertexBuffer> GetBoneInfluenceBuffer() { return m_BoneInfluenceBuffer; }
		std::filesystem::path GetFilePath() { return m_FilePath; }
		std::vector<std::string> GetAnimationNames() const { return m_AnimationNames; }
		virtual ~MeshSource();
		std::vector<AssetHandle> m_Materials;
		const MeshNode& GetRootNode() const { return m_Nodes[0]; }
		const std::vector<MeshNode>& GetNodes() const { return m_Nodes; }

	private:
		std::vector<Submesh> m_Submeshes;
		std::vector<Vertex> m_Vertices;
		std::vector<Index> m_Indices;
		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;
		std::vector<MeshNode> m_Nodes;
		mutable Scope<Skeleton> m_Skeleton; // mutable��ʾ��ʹ��const������Ҳ�����޸���
		std::vector<std::string> m_AnimationNames;
		mutable std::vector<Scope<Animation>> m_Animations;

		std::vector<BoneInfluence> m_BoneInfluences; // ÿ�������Ӧ�Ĺ���Ӱ������
		std::vector<BoneInfo> m_BoneInfo; // ������Ϣ


		std::filesystem::path m_FilePath;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<VertexBuffer> m_BoneInfluenceBuffer;	
		Ref<IndexBuffer> m_IndexBuffer;
		AABB m_BoundingBox;
		friend class AssimpMeshImporter;
		friend class MaterialAsset;
		friend class Skeleton;
		friend class SceneRender;
	};

}
