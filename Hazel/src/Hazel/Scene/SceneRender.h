#pragma once
#include "Scene.h"
#include "Hazel/Asset/Model/Mesh.h"
namespace Hazel {
	class SceneRender : public RefCounted
	{
	public:
		SceneRender();
		Ref<Image2D> GetFinalImage() { return m_GridFrameBuffer->GetImage(0); }
		void SetScene(Scene* scene) { m_scene = scene; }
		void PreRender(EditorCamera& camera);
		void EndRender();
		void SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform);
		void SubmitMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms);
		void SetViewprotSize(float width, float height);

	private:
		// Pass
		void GeoPass();

		// Grid
		void GridPass();
	private:
		void Init();
		void Draw();
		void GeoAnimPass();
		void UpdateVPMatrix(EditorCamera& camera);
	private:
		float ViewportWidth = 1216.0f, ViewportHeight = 849.0f;

		VertexBufferLayout vertexLayout = {
			{ ShaderDataType::Float3, "a_Position" },
			{ ShaderDataType::Float3, "a_Normal" },
			{ ShaderDataType::Float3, "a_Tangent" },
			{ ShaderDataType::Float3, "a_Binormal" },
			{ ShaderDataType::Float2, "a_TexCoord" }
		};

		VertexBufferLayout instanceLayout = {
			{ ShaderDataType::Float4, "a_MRow0" },
			{ ShaderDataType::Float4, "a_MRow1" },
			{ ShaderDataType::Float4, "a_MRow2" },
		};
		VertexBufferLayout boneInfluenceLayout = {
			{ ShaderDataType::Int4,   "a_BoneIDs" },
			{ ShaderDataType::Float4, "a_BoneWeights" },
		};
		struct MeshKey
		{
			AssetHandle MeshHandle;
			AssetHandle MaterialHandle;
			uint32_t SubmeshIndex;
			bool IsSelected;

			MeshKey(AssetHandle meshHandle, AssetHandle materialHandle, uint32_t submeshIndex, bool isSelected)
				: MeshHandle(meshHandle), MaterialHandle(materialHandle), SubmeshIndex(submeshIndex), IsSelected(isSelected)
			{
			}

			bool operator<(const MeshKey& other) const
			{
				if (MeshHandle < other.MeshHandle)
					return true;

				if (MeshHandle > other.MeshHandle)
					return false;

				if (SubmeshIndex < other.SubmeshIndex)
					return true;

				if (SubmeshIndex > other.SubmeshIndex)
					return false;

				if (MaterialHandle < other.MaterialHandle)
					return true;

				if (MaterialHandle > other.MaterialHandle)
					return false;

				return IsSelected < other.IsSelected;

			}
		};
		void CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<glm::mat4>& boneTransforms);

		struct StaticDrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
		};
		struct DrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
			uint32_t InstanceOffset = 0;
			bool IsRigged = false;
		};
		std::map<MeshKey, DrawCommand> m_DrawList;
		std::map<MeshKey, StaticDrawCommand> m_StaticMeshDrawList;
		struct TransformVertexData
		{
			glm::vec4 MRow[3]; // 4行4列存储Model变换矩阵，最后一行不存，因为固定是0001
		};
		struct TransformBuffer
		{
			Ref<VertexBuffer> Buffer;  //用顶点数据来传递Model变换矩阵
			TransformVertexData* Data = nullptr; // 具体的数据
		};
		std::vector<TransformBuffer> m_SubmeshTransformBuffers;

		struct TransformMapData
		{
			std::vector<TransformVertexData> Transforms;
			uint32_t TransformOffset = 0;
		};
		std::map<MeshKey, TransformMapData> m_MeshTransformMap;  // 每个MeshKey对应多个实例的变换矩阵


		using BoneTransforms = std::array<glm::mat4, 100>; // Note: 100 == MAX_BONES from the shaders
		struct BoneTransformsMapData
		{
			std::vector<BoneTransforms> BoneTransformsData;
			uint32_t BoneTransformsBaseIndex = 0;
		};	
		std::map<MeshKey, BoneTransformsMapData> m_MeshBoneTransformsMap;
		BoneTransforms* m_BoneTransformsData = nullptr;
		Ref<StorageBufferSet> m_SBSBoneTransforms;


	private:
		bool NeedResize = false;
		Scene* m_scene;
		// command buffer
		Ref<RenderCommandBuffer> m_CommandBuffer;

		// uniform buffer
		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			float Width;
			float Height;
			float Near;
			float Far;
		};
		CameraData* m_CameraData = nullptr;
		Ref<UniformBufferSet> m_CameraDataBufferSet;


		// GeoPass
		Ref<RenderPass> m_GeoPass;
		Ref<Pipeline> m_GeoPipeline;
		Ref<Framebuffer> m_GeoFrameBuffer;
		// GeoAnimPass
		Ref<RenderPass> m_GeoAnimPass;
		Ref<Pipeline> m_GeoAnimPipeline;
		Ref<Framebuffer> m_GeoAnimFrameBuffer;
		// GridPass
		Ref<RenderPass> m_GridPass;
		Ref<Pipeline> m_GridPipeline;
		Ref<Framebuffer> m_GridFrameBuffer;
	};

}
