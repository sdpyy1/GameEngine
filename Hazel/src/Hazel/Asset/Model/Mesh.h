#pragma once
#include "Hazel/Asset/Asset.h"
#include "Hazel/Asset/AssetManager.h"
#include <Hazel/Math/AABB.h>
#include <Hazel/Renderer/VertexBuffer.h>
#include <Hazel/Renderer/IndexBuffer.h>
#include "Hazel/Asset/Model/Material.h"
#include "Hazel/Asset/Model/MaterialAsset.h"

namespace Hazel {
	struct Vertex {
		glm::vec3 Position;  // 所有分量为 0
		glm::vec3 Normal;
		glm::vec3 Tangent;
		glm::vec3 Binormal;
		glm::vec2 Texcoord;
		// 用于创建pipeline
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
		glm::mat4 LocalTransform{ 1.0f };
		AABB BoundingBox;

		std::string NodeName, MeshName;
		bool IsRigged = false;
	};
	class MeshSource : public Asset {
	public:
		MeshSource() = default;
		Ref<VertexBuffer> GetVertexBuffer() {return m_VertexBuffer;}
		Ref<IndexBuffer> GetIndexBuffer() {return m_IndexBuffer;}
		virtual ~MeshSource();
		std::vector<AssetHandle> m_Materials;
	private:
		std::vector<Submesh> m_Submeshes;
		std::vector<Vertex> m_Vertices;
		std::vector<Index> m_Indices;
		std::unordered_map<uint32_t, std::vector<Triangle>> m_TriangleCache;
		std::vector<MeshNode> m_Nodes;

		Ref<VertexBuffer> m_VertexBuffer;
		Ref<VertexBuffer> m_BoneInfluenceBuffer;
		Ref<IndexBuffer> m_IndexBuffer;
		AABB m_BoundingBox;
		friend class AssimpMeshImporter;
		friend class MaterialAsset;
	};

}
