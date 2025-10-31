#pragma once
#include "Scene.h"
#include "Hazel/Asset/Model/Mesh.h"
#include <Hazel/Renderer/ComputePass.h>
namespace Hazel {
	class SceneRender : public RefCounted
	{
	public:
		SceneRender();
		Ref<Image2D> GetFinalImage() { return m_GridFrameBuffer->GetImage(0); }
		void PreRender(SceneInfo sceneData);
		void EndRender();
		void SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform);
		void SubmitDynamicMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms);
		void SetViewprotSize(float width, float height);

	private:
		// Shadow
		void ShadowPass();
		// Geo
		void GeoPass();
		// Grid
		void GridPass();
	private:
		void Init();
		void Draw();
		void InitBuffers();
		void UploadCameraData();
		void InitDirShadowPass();
		void InitGeoPass();
		void InitGridPass();
		void UploadMeshAndBoneTransForm();
		void UploadCSMShadowData();
		void InitPreDepthPass();

		void PreDepthPass();

		void HandleResizeRuntime();
		void UploadDescriptorRuntime();

		void ClearPass(Ref<RenderPass> renderPass, bool explicitClear = false);
		void InitSpotShadowPass();
		void UploadSpotShadowData();
		void SpotShadowPass();
	private:
		SceneInfo* m_SceneData = nullptr;
		uint32_t shadowMapResolution = 4096;
		uint32_t NumShadowCascades = 4;
		float m_ViewportWidth = 1216.0f, m_ViewportHeight = 849.0f;

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
		struct CascadeData
		{
			glm::mat4 ViewProj;
			glm::mat4 View;
			float SplitDepth;
		};
		glm::vec4 CascadeSplits;

		void CalculateCascades(CascadeData* cascades, const EditorCamera& sceneCamera, const glm::vec3& lightDirection) const;

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
		struct DynamicDrawCommand
		{
			Ref<MeshSource> MeshSource;
			uint32_t SubmeshIndex;
			Ref<MaterialAsset> MaterialAsset;
			uint32_t InstanceCount = 0;
			uint32_t InstanceOffset = 0;
			bool IsRigged = false;
		};
		std::map<MeshKey, DynamicDrawCommand> m_DynamicDrawList;
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
		std::vector<Ref<Material>> m_HZBMaterials; // per-mip

		struct UBShadow
		{
			glm::mat4 ViewProjection[4];
		};
	private:
		bool NeedResize = false;
		// command buffer
		Ref<RenderCommandBuffer> m_CommandBuffer;
		struct UBSpotShadowData
		{
			glm::mat4 ShadowMatrices[1000]{};
		} SpotShadowDataUB;
		// uniform buffer
		struct CameraData {
			glm::mat4 view;
			glm::mat4 proj;
			float Width;
			float Height;
			float Near;
			float Far;
		};
		struct UBSpotLights
		{
			uint32_t Count{ 0 };
			glm::vec3 Padding{};
			SpotLight SpotLights[1000]{};
		} SpotLightUB;
		CameraData* m_CameraData = nullptr;
		Ref<UniformBufferSet> m_UBSCameraData;
		UBShadow* m_ShadowData = nullptr;
		Ref<UniformBufferSet> m_UBSShadow;
		Ref<UniformBufferSet> m_UBSSpotShadowData;
		UBSpotLights* spotLightData = nullptr;
		// Hierarchical Depth
		struct HierarchicalDepthTexture
		{
			Ref<Texture2D> Texture;
			std::vector<Ref<ImageView>> ImageViews; // per-mip
		} m_HierarchicalDepthTexture;

		// shadowPass
		std::vector<Ref<RenderPass>> m_DirectionalShadowMapPass; // Per-cascade
		std::vector<Ref<RenderPass>> m_DirectionalShadowMapAnimPass; // Per-cascade
		Ref<Pipeline> m_ShadowPassPipelines[4];
		Ref<Pipeline> m_ShadowPassPipelinesAnim[4];

		Ref<Pipeline> m_SpotShadowPassPipeline;
		Ref<Pipeline> m_SpotShadowPassAnimPipeline;
		Ref<RenderPass> m_SpotShadowPass;
		Ref<RenderPass> m_SpotShadowAnimPass;

		//PreDepthPass
		Ref<Pipeline> m_PreDepthPipeline;
		Ref<Pipeline> m_PreDepthPipelineAnim;
		Ref<RenderPass> m_PreDepthPass, m_PreDepthAnimPass;
		Ref<Framebuffer> m_PreDepthClearFramebuffer, m_PreDepthLoadFramebuffer;
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

		// HierarchicalDepthPass
		Ref<ComputePass> m_HierarchicalDepthPass;

		// 预计算Pass
		Ref<ComputePass> m_EquirectangularPass;
		Ref<TextureCube> m_EnvCubeMap;
		Ref<TextureCube> m_EnvPreFilterMap;
		Ref<TextureCube> m_EnvIrradianceMap;
		Ref<Texture2D> m_EnvEquirect;
		Ref<Texture2D> m_EnvLut;
		Ref<ComputePass> m_EnvironmentIrradiancePass;
		Ref<ComputePass> m_EnvironmentPreFilterPass;
		void InitHZBPass();
		void HZBComputePass();
		void HandleHZBResize();
		void preComputeEnv();
		void InitEnvNeed();
	};
}
