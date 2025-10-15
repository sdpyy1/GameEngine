#pragma once
#include "Scene.h"
#include "Hazel/Asset/Model/Mesh.h"
namespace Hazel {
	class SceneRender : public RefCounted
	{
	public:
		SceneRender();
		Ref<Image2D> GetFinalImage() { return m_GeoFrameBuffer->GetImage(2); }
		void SetScene(Scene* scene) { m_scene = scene; }
		void PreRender(EditorCamera& camera);
		void EndRender();
		void SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform);

	private:
		// Pass
		void GeoPass();

		// Grid
		void GridPass();
	private:
		void Init();
		void Draw();
		void UpdateVPMatrix(EditorCamera& camera);
	private:
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
		struct StaticDrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
		};

		std::map<MeshKey, StaticDrawCommand> m_StaticMeshDrawList;
		struct TransformVertexData
		{
			glm::vec4 MRow[3]; // 4行4列存储Model变换矩阵
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
		std::map<MeshKey, TransformMapData> m_MeshTransformMap;

	private:
		Scene* m_scene;
		// command buffer
		Ref<RenderCommandBuffer> m_CommandBuffer;

		// uniform buffer
		struct UniformBufferObject {
			glm::mat4 view;
			glm::mat4 proj;
			float width;
			float height;
		};
		UniformBufferObject* m_VPMatrix = nullptr;
		Ref<UniformBufferSet> m_VPUniformBufferSet;


		// GeoPass
		Ref<RenderPass> m_GeoPass;
		Ref<Pipeline> m_GeoPipeline;
		Ref<Framebuffer> m_GeoFrameBuffer;

		// GridPass
		Ref<RenderPass> m_GridPass;
		Ref<Pipeline> m_GridPipeline;
		Ref<Framebuffer> m_GridFrameBuffer;
	};

}
