#include "hzpch.h"
#include "SceneRender.h"
#include "Components.h"
#include "Hazel/Asset/AssetManager.h"
#include "Entity.h"
#include <Hazel/Asset/AssetImporter.h>
#include <glm/gtx/compatibility.hpp>
#include <Hazel/Asset/Model/Mesh.h>
#include <glm/gtc/matrix_transform.hpp>
#include <Hazel/Math/Math.h>
#include <Platform/Vulkan/VulkanRenderCommandBuffer.h>
namespace Hazel {
	SceneRender::SceneRender()
	{
		Init();
	}
	void SceneRender::InitHZBPass()
	{
		TextureSpecification spec;
		spec.Format = ImageFormat::RED32F;
		spec.Width = 1;
		spec.Height = 1;
		spec.SamplerWrap = TextureWrap::Clamp;
		spec.SamplerFilter = TextureFilter::Nearest;
		spec.DebugName = "HierarchicalZ";
		m_HierarchicalDepthTexture.Texture = Texture2D::Create(spec); // 创建了一张纹理
		Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HZB");
		ComputePassSpecification hdPassSpec;
		hdPassSpec.DebugName = "HierarchicalDepth";
		hdPassSpec.Pipeline = PipelineCompute::Create(shader);
		m_HierarchicalDepthPass = ComputePass::Create(hdPassSpec);
	}
	void SceneRender::HZBComputePass()
	{
		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		Renderer::BeginComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
		auto ReduceHZB = [commandBuffer = m_CommandBuffer, hierarchicalDepthPass = m_HierarchicalDepthPass, hierarchicalDepthTexture = m_HierarchicalDepthTexture.Texture, hzbMaterials = m_HZBMaterials]
		(const uint32_t startDestMip, const uint32_t parentMip, const glm::vec2& DispatchThreadIdToBufferUV, const glm::vec2& InputViewportMaxBound, const bool isFirstPass)
			{
				struct HierarchicalZComputePushConstants
				{
					glm::vec2 DispatchThreadIdToBufferUV;
					glm::vec2 InputViewportMaxBound;
					glm::vec2 InvSize;
					int FirstLod;
					bool IsFirstPass;
					char Padding[3]{ 0, 0, 0 };
				} hierarchicalZComputePushConstants;

				hierarchicalZComputePushConstants.IsFirstPass = isFirstPass;
				hierarchicalZComputePushConstants.FirstLod = (int)startDestMip;
				hierarchicalZComputePushConstants.DispatchThreadIdToBufferUV = DispatchThreadIdToBufferUV;
				hierarchicalZComputePushConstants.InputViewportMaxBound = InputViewportMaxBound;

				const glm::ivec2 srcSize(Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << parentMip));
				const glm::ivec2 dstSize(Math::DivideAndRoundUp(hierarchicalDepthTexture->GetSize(), 1u << startDestMip));
				hierarchicalZComputePushConstants.InvSize = glm::vec2{ 1.0f / (float)srcSize.x, 1.0f / (float)srcSize.y };

				glm::uvec3 workGroups(Math::DivideAndRoundUp(dstSize.x, 8), Math::DivideAndRoundUp(dstSize.y, 8), 1);
				Renderer::DispatchCompute(commandBuffer, hierarchicalDepthPass, hzbMaterials[startDestMip / 4], workGroups, Buffer(&hierarchicalZComputePushConstants, sizeof(hierarchicalZComputePushConstants)));
			};
		// Reduce first 4 mips
		glm::ivec2 srcSize = m_PreDepthPass->GetDepthOutput()->GetSize();
		ReduceHZB(0, 0, { 1.0f / glm::vec2{ srcSize } }, { (glm::vec2{ srcSize } - 0.5f) / glm::vec2{ srcSize } }, true);

		// Reduce the next mips
		for (uint32_t startDestMip = maxMipBatchSize; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize)
		{
			srcSize = Math::DivideAndRoundUp(m_HierarchicalDepthTexture.Texture->GetSize(), 1u << uint32_t(startDestMip - 1));
			ReduceHZB(startDestMip, startDestMip - 1, { 2.0f / glm::vec2{ srcSize } }, glm::vec2{ 1.0f }, false);
		}		
		Renderer::EndComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
	}

	void SceneRender::Init()
	{
		m_CommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		InitBuffers();
		InitDirShadowPass();
		InitSpotShadowPass();
		InitPreDepthPass();
		InitHZBPass();
		InitGeoPass();
		InitGridPass();
	}

	void SceneRender::Draw() {
		ShadowPass();
		SpotShadowPass();
		PreDepthPass();
		HZBComputePass();
		GeoPass();
		GridPass();
	}
	void SceneRender::PreRender(SceneInfo sceneData)
	{	
		m_SceneData = &sceneData;
		// 注意如果Resize里有图片被当作了资源描述符资源，销毁的资源需要在UploadDescriptorRuntime()中重新绑定（会自动判断是否需要更新）
		HandleResizeRuntime();
		UploadDescriptorRuntime();
		// 更新各种资源信息
		UploadCameraData(); // 摄像机数据
		UploadMeshAndBoneTransForm(); // 模型变换和骨骼变换矩阵
		UploadCSMShadowData(); // 级联阴影数据
		UploadSpotShadowData(); // 聚光阴影 TODO：待完成
		HandleHZBTexture();
	}
	void SceneRender::EndRender()
	{
		m_CommandBuffer->Begin();
		Draw(); // 执行渲染命令
		m_CommandBuffer->End();
		m_CommandBuffer->Submit();
		m_DynamicDrawList.clear();
		m_StaticMeshDrawList.clear();
		m_MeshTransformMap.clear();
		m_MeshBoneTransformsMap.clear();
	}

	void SceneRender::GeoPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		// 静态网格
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, false);
		for (auto& [meshKey, drawCommand] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey); 
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline,drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, drawCommand.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);

		// 动态网格
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoAnimPass, false);
		for (auto& [meshKey, drawCommand] : m_DynamicDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey);
			if (drawCommand.IsRigged) {
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(meshKey);
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_GeoAnimPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, drawCommand.InstanceCount);
			}
			else {
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_GeoAnimPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, drawCommand.InstanceCount);

			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::CalculateCascades(CascadeData* cascades, const EditorCamera& sceneCamera, const glm::vec3& lightDirection) const
	{
		float CascadeSplitLambda = 0.92f;
		float CascadeFarPlaneOffset = 50.0f, CascadeNearPlaneOffset = -50.0f;
		float m_ScaleShadowCascadesToOrigin = 0.0f;
		float scaleToOrigin = m_ScaleShadowCascadesToOrigin;

		glm::mat4 viewMatrix = sceneCamera.GetViewMatrix();
		constexpr glm::vec4 origin = glm::vec4(glm::vec3(0.0f), 1.0f);
		viewMatrix[3] = glm::lerp(viewMatrix[3], origin, scaleToOrigin);

		auto viewProjection = sceneCamera.GetUnReversedProjectionMatrix() * viewMatrix;

		const int SHADOW_MAP_CASCADE_COUNT = 4;
		float cascadeSplits[SHADOW_MAP_CASCADE_COUNT];

		float nearClip = sceneCamera.GetNearClip();
		float farClip = sceneCamera.GetFarClip();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		// Calculate split depths based on view camera frustum
		// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = CascadeSplitLambda * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		cascadeSplits[3] = 0.3f;

		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] =
			{
				glm::vec3(-1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f,  1.0f, -1.0f),
				glm::vec3(1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f, -1.0f, -1.0f),
				glm::vec3(-1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f,  1.0f,  1.0f),
				glm::vec3(1.0f, -1.0f,  1.0f),
				glm::vec3(-1.0f, -1.0f,  1.0f),
			};

			// Project frustum corners into world space
			glm::mat4 invCam = glm::inverse(viewProjection);
			for (uint32_t i = 0; i < 8; i++)
			{
				glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[i], 1.0f);
				frustumCorners[i] = invCorner / invCorner.w;
			}

			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// Get frustum center
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			//frustumCenter *= 0.01f;

			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 maxExtents = glm::vec3(radius);
			glm::vec3 minExtents = -maxExtents;

			glm::vec3 lightDir = -lightDirection;
			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 0.0f, 1.0f));
			glm::mat4 lightOrthoMatrix = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f + CascadeNearPlaneOffset, maxExtents.z - minExtents.z + CascadeFarPlaneOffset);

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_PreDepthLoadFramebuffer->GetWidth();

			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;

			// Store split distance and matrix in cascade
			//cascades[i].SplitDepth = (nearClip + splitDist * clipRange) * -1.0f;
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange);
			cascades[i].ViewProj = lightOrthoMatrix * lightViewMatrix;
			cascades[i].View = lightViewMatrix;

			lastSplitDist = cascadeSplits[i];
		}
	}
	void SceneRender::InitSpotShadowPass() // TODO：这里和别的Pass不一样
	{
		FramebufferSpecification framebufferSpec;
		framebufferSpec.Width = shadowMapResolution;
		framebufferSpec.Height = shadowMapResolution;
		framebufferSpec.Attachments = { ImageFormat::DEPTH32F };
		framebufferSpec.DepthClearValue = 1.0f;
		framebufferSpec.NoResize = true;
		framebufferSpec.DebugName = "SpotShadowMap";

		auto shadowPassShader = Renderer::GetShaderLibrary()->Get("SpotShadowMap");
		auto shadowPassShaderAnim = Renderer::GetShaderLibrary()->Get("SpotShadowMapAnim");

		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "SpotShadowPass";
		pipelineSpec.Shader = shadowPassShader;
		pipelineSpec.TargetFramebuffer = Framebuffer::Create(framebufferSpec);
		pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;
		PipelineSpecification pipelineSpecAnim = pipelineSpec;
		pipelineSpecAnim.DebugName = "SpotShadowPassAnim";
		pipelineSpecAnim.Shader = shadowPassShaderAnim;
		pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;

		m_SpotShadowPassPipeline = Pipeline::Create(pipelineSpec);
		m_SpotShadowPassAnimPipeline = Pipeline::Create(pipelineSpecAnim);

		RenderPassSpecification spotShadowPassSpec;
		spotShadowPassSpec.DebugName = "SpotShadowMap";
		spotShadowPassSpec.Pipeline = m_SpotShadowPassPipeline;
		m_SpotShadowPass = RenderPass::Create(spotShadowPassSpec);
		spotShadowPassSpec.DebugName = "SpotShadowMapAnim";
		spotShadowPassSpec.Pipeline = m_SpotShadowPassAnimPipeline;
		m_SpotShadowAnimPass = RenderPass::Create(spotShadowPassSpec);
		m_SpotShadowPass->SetInput(m_UBSSpotShadowData, 0);
		m_SpotShadowAnimPass->SetInput(m_UBSSpotShadowData, 0);
		m_SpotShadowAnimPass->SetInput(m_SBSBoneTransforms, 1, true);
	}

	void SceneRender::UploadSpotShadowData()
	{
		const std::vector<SpotLight>& spotLightsVec = m_SceneData->SceneLightEnvironment.SpotLights;
		// TODO:待完成
	}

	void SceneRender::SpotShadowPass()
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}
	void SceneRender::UploadCSMShadowData() {
		if (m_SceneData->SceneLightEnvironment.DirectionalLights[0].Intensity == 0) return;
		CascadeData cascades[4];
		CalculateCascades(cascades, m_SceneData->camera, m_SceneData->SceneLightEnvironment.DirectionalLights[0].Direction);
		for (int i = 0; i < NumShadowCascades; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			m_ShadowData->ViewProjection[i] = cascades[i].ViewProj;
		}
		m_UBSShadow->Get()->SetData(m_ShadowData, sizeof(UBShadow));
	}
	void SceneRender::ShadowPass() {
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		auto& directionalLights = m_SceneData->SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Intensity == 0.0f || !directionalLights[0].CastShadows)
		{
			return;
		}
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_StaticMeshDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelines[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount, cascade);
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{
			Renderer::BeginRenderPass(m_CommandBuffer, m_DirectionalShadowMapAnimPass[i]);

			// Render entities
			const Buffer cascade(&i, sizeof(uint32_t));
			for (auto& [mk, dc] : m_DynamicDrawList)
			{
				const auto& transformData = m_MeshTransformMap.at(mk);
				if (dc.IsRigged)
				{
					const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount, cascade);
				}
				else {
					Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_ShadowPassPipelinesAnim[i], dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount, cascade);
				}
			}
			Renderer::EndRenderPass(m_CommandBuffer);
		}
	}

	void SceneRender::InitPreDepthPass() {
		FramebufferSpecification preDepthFramebufferSpec;
		preDepthFramebufferSpec.DebugName = "PreDepth";
		preDepthFramebufferSpec.Attachments = { ImageFormat::Depth };
		preDepthFramebufferSpec.DepthClearValue = 1.0f;
		m_PreDepthClearFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);
		preDepthFramebufferSpec.ClearDepthOnLoad = false;
		preDepthFramebufferSpec.ExistingImages[0] = m_PreDepthClearFramebuffer->GetDepthImage();
		preDepthFramebufferSpec.DebugName = "PreDepthAnim";
		m_PreDepthLoadFramebuffer = Framebuffer::Create(preDepthFramebufferSpec);
		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "PreDepth";
		pipelineSpec.TargetFramebuffer = m_PreDepthClearFramebuffer;
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepth");
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;
		m_PreDepthPipeline = Pipeline::Create(pipelineSpec);
		pipelineSpec.TargetFramebuffer = m_PreDepthLoadFramebuffer;
		pipelineSpec.DebugName = "PreDepth-Anim";
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("PreDepthAnim");
		pipelineSpec.BoneInfluenceLayout = boneInfluenceLayout;
		m_PreDepthPipelineAnim = Pipeline::Create(pipelineSpec);
		RenderPassSpecification preDepthRenderPassSpec;
		preDepthRenderPassSpec.DebugName = "PreDepth";
		preDepthRenderPassSpec.Pipeline = m_PreDepthPipeline;
		m_PreDepthPass = RenderPass::Create(preDepthRenderPassSpec);
		preDepthRenderPassSpec.DebugName = "PreDepth-Anim";
		preDepthRenderPassSpec.Pipeline = m_PreDepthPipelineAnim;
		m_PreDepthAnimPass = RenderPass::Create(preDepthRenderPassSpec);

		m_PreDepthPass->SetInput(m_UBSCameraData, 0);
		m_PreDepthAnimPass->SetInput(m_UBSCameraData, 0);
		m_PreDepthAnimPass->SetInput(m_SBSBoneTransforms, 1, true);
	}
	void SceneRender::PreDepthPass() {
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthPass);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_PreDepthPipeline, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);
		Renderer::BeginRenderPass(m_CommandBuffer, m_PreDepthAnimPass);
		for (auto& [mk, dc] : m_DynamicDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(mk);
			if (dc.IsRigged)
			{
				const auto& boneTransformsData = m_MeshBoneTransformsMap.at(mk);
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, dc.InstanceCount);
			}
			else {
				Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_PreDepthPipelineAnim, dc.MeshSource, dc.SubmeshIndex, nullptr, m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, 0, dc.InstanceCount);
			}
		}
		Renderer::EndRenderPass(m_CommandBuffer);

	}
	void SceneRender::GridPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_GridPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer,6);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	
	void SceneRender::UploadCameraData()
	{
		m_CameraData->view = m_SceneData->camera.GetViewMatrix();
		m_CameraData->proj = m_SceneData->camera.GetProjectionMatrix();
		m_CameraData->proj[1][1] *= -1; // Y轴反转
		m_CameraData->Near = m_SceneData->camera.GetNearClip();
		m_CameraData->Far = m_SceneData->camera.GetFarClip();
		m_CameraData->Width = m_SceneData->camera.GetViewportWidth();
		m_CameraData->Height = m_SceneData->camera.GetViewportHeight();
		m_UBSCameraData->Get()->SetData((void*)m_CameraData, sizeof(CameraData));
	}
	void SceneRender::SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform) {
		const auto& submeshData = meshSource->GetSubmeshes();
		for(uint32_t submeshIndex = 0; submeshIndex <submeshData.size(); submeshIndex++){
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;  // subMesh的全局变换
			AssetHandle materialHandle = meshSource->GetMaterialHandle(submeshData[submeshIndex].MaterialIndex);
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);// subMesh的材质索引;
			HZ_CORE_VERIFY(material);
			MeshKey meshKey = { meshSource->Handle, materialHandle, submeshIndex, false };
			// 缓存变换矩阵
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back(); // 对于每一种MeshKey，都存储多个Transforms矩阵，表示多个实例
			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };
			// 缓存绘制命令
			StaticDrawCommand& drawCommand = m_StaticMeshDrawList[meshKey];
			drawCommand.MeshSource = meshSource;
			drawCommand.SubmeshIndex = submeshIndex;
			drawCommand.MaterialAsset = material;
			drawCommand.InstanceCount++;
		}
	};
	void SceneRender::SubmitDynamicMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms)
	{
		// TODO: Culling, sorting, etc.

		const auto& submeshes = meshSource->GetSubmeshes();
		const auto& submesh = submeshes[submeshIndex];
		uint32_t materialIndex = submesh.MaterialIndex;
		bool isRigged = submesh.IsRigged;

		AssetHandle materialHandle = meshSource->GetMaterialHandle(submeshes[submeshIndex].MaterialIndex);
		Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);

		MeshKey meshKey = { meshSource->Handle, materialHandle, submeshIndex, false };
		auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back();

		transformStorage.MRow[0] = { transform[0][0], transform[1][0], transform[2][0], transform[3][0] };
		transformStorage.MRow[1] = { transform[0][1], transform[1][1], transform[2][1], transform[3][1] };
		transformStorage.MRow[2] = { transform[0][2], transform[1][2], transform[2][2], transform[3][2] };

		if (isRigged)
		{
			CopyToBoneTransformStorage(meshKey, meshSource, boneTransforms);
		}
		// Main geo
		{
			DynamicDrawCommand& drawCommand = m_DynamicDrawList[meshKey];
			drawCommand.MeshSource = meshSource;
			drawCommand.SubmeshIndex = submeshIndex;
			drawCommand.InstanceCount++;
			drawCommand.MaterialAsset = material;
			drawCommand.IsRigged = isRigged;  // TODO: would it be better to have separate draw list for rigged meshes, or this flag is OK?
		}
	}
	void SceneRender::CopyToBoneTransformStorage(const MeshKey& meshKey, const Ref<MeshSource>& meshSource, const std::vector<glm::mat4>& boneTransforms)
	{
		auto& boneTransformStorage = m_MeshBoneTransformsMap[meshKey].BoneTransformsData.emplace_back();
		if (boneTransforms.empty())
		{
			boneTransformStorage.fill(glm::identity<glm::mat4>());
		}
		else
		{
			for (size_t i = 0; i < meshSource->m_BoneInfo.size(); ++i)
			{
				// 1. 逆变换矩阵转移到骨骼空间 2. 跟随骨骼在世界空间移动 3. 全局变换
				boneTransformStorage[i] = meshSource->GetSkeleton()->GetTransform() * boneTransforms[meshSource->m_BoneInfo[i].BoneIndex] * meshSource->m_BoneInfo[i].InverseBindPose;
			}
		}
	}
	void SceneRender::SetViewprotSize(float width, float height) {
		if(width != m_ViewportWidth || height != m_ViewportHeight)
		{
			m_ViewportWidth = width;
			m_ViewportHeight = height;
			NeedResize = true;
		}
	}
	void SceneRender::InitBuffers()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		// MVP矩阵的UBO
		m_CameraData = new CameraData();
		m_UBSCameraData = UniformBufferSet::Create(sizeof(CameraData), "CameraData");
		m_ShadowData = new UBShadow();
		m_UBSShadow = UniformBufferSet::Create(sizeof(UBShadow), "Shadow");
		m_UBSSpotShadowData = UniformBufferSet::Create(sizeof(UBSpotShadowData),"SportShadowData");
		// 用一个很大的顶点缓冲区来存储所有的变换矩阵
		const size_t TransformBufferCount = 10 * 1024; // 10240 transforms
		m_SubmeshTransformBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_SubmeshTransformBuffers[i].Buffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount, "TransformBuffers");
			m_SubmeshTransformBuffers[i].Data = new TransformVertexData[TransformBufferCount];
		}
		{
			StorageBufferSpecification spec;
			spec.DebugName = "BoneTransforms";
			spec.GPUOnly = false;
			const size_t BoneTransformBufferCount = 1 * 1024; // basically means limited to 1024 animated meshes   TODO(0x): resizeable/flushable
			m_SBSBoneTransforms = StorageBufferSet::Create(spec, sizeof(BoneTransforms) * BoneTransformBufferCount);
			m_BoneTransformsData = new BoneTransforms[BoneTransformBufferCount];
		}
	}
	void SceneRender::InitDirShadowPass() {
		ImageSpecification spec;
		spec.Format = ImageFormat::DEPTH32F;
		spec.Usage = ImageUsage::Attachment;
		spec.Width = shadowMapResolution;
		spec.Height = shadowMapResolution;
		spec.Layers = NumShadowCascades;
		spec.DebugName = "ShadowCascades";
		Ref<Image2D> cascadedDepthImage = Image2D::Create(spec);
		cascadedDepthImage->Invalidate();
		if (NumShadowCascades > 1)
			cascadedDepthImage->CreatePerLayerImageViews();
		FramebufferSpecification shadowMapFramebufferSpec;
		shadowMapFramebufferSpec.DebugName = "Dir Shadow Map";
		shadowMapFramebufferSpec.Width = shadowMapResolution;
		shadowMapFramebufferSpec.Height = shadowMapResolution;
		shadowMapFramebufferSpec.Attachments = { ImageFormat::DEPTH32F };
		shadowMapFramebufferSpec.ClearColor = { 0.0f, 0.0f, 0.0f, 0.0f };
		shadowMapFramebufferSpec.DepthClearValue = 1.0f;
		shadowMapFramebufferSpec.NoResize = true;
		shadowMapFramebufferSpec.ExistingImage = cascadedDepthImage;

		PipelineSpecification pipelineSpec;
		pipelineSpec.DebugName = "DirShadowPass";
		pipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("DirShadowMap");
		pipelineSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
		pipelineSpec.Layout = vertexLayout;
		pipelineSpec.InstanceLayout = instanceLayout;

		PipelineSpecification pipelineSpecAnim = pipelineSpec;
		pipelineSpecAnim.DebugName = "DirShadowPass-Anim";
		pipelineSpecAnim.Shader = Renderer::GetShaderLibrary()->Get("DirShadowMapAnim");
		pipelineSpecAnim.BoneInfluenceLayout = boneInfluenceLayout;
		RenderPassSpecification shadowMapRenderPassSpec;
		m_DirectionalShadowMapPass.resize(NumShadowCascades);
		m_DirectionalShadowMapAnimPass.resize(NumShadowCascades);
		for (uint32_t i = 0; i < NumShadowCascades; i++)
		{		shadowMapRenderPassSpec.DebugName = shadowMapFramebufferSpec.DebugName;

			shadowMapFramebufferSpec.ExistingImageLayers.clear();
			shadowMapFramebufferSpec.ExistingImageLayers.emplace_back(i);

			shadowMapFramebufferSpec.ClearDepthOnLoad = true;
			pipelineSpec.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);

			m_ShadowPassPipelines[i] = Pipeline::Create(pipelineSpec);
			shadowMapRenderPassSpec.DebugName = "DirShadowPass";
			shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelines[i];

			shadowMapFramebufferSpec.ClearDepthOnLoad = false;
			pipelineSpecAnim.TargetFramebuffer = Framebuffer::Create(shadowMapFramebufferSpec);
			m_ShadowPassPipelinesAnim[i] = Pipeline::Create(pipelineSpecAnim);

			m_DirectionalShadowMapPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			m_DirectionalShadowMapPass[i]->SetInput(m_UBSShadow, 0);
			//HZ_CORE_VERIFY(m_DirectionalShadowMapPass[i]->Validate());
			//m_DirectionalShadowMapPass[i]->Bake();
			shadowMapRenderPassSpec.DebugName = "DirShadowPassAnim";
			shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelinesAnim[i];
			m_DirectionalShadowMapAnimPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			m_DirectionalShadowMapAnimPass[i]->SetInput(m_UBSShadow, 0);
			m_DirectionalShadowMapAnimPass[i]->SetInput(m_SBSBoneTransforms, 1,true); // TODO:注意绑定点
		}
	}
	void SceneRender::InitGeoPass() {

		// GeoPass
		{
			FramebufferSpecification framebufferSpec;
			// pos normal albedo mr depth
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBuffer";
			framebufferSpec.ExistingImages[4] = m_PreDepthLoadFramebuffer->GetDepthImage();
			framebufferSpec.ClearDepthOnLoad = false;
			m_GeoFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBuffer");
			pSpec.TargetFramebuffer = m_GeoFrameBuffer;
			pSpec.DebugName = "GbufferPipeline";
			pSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
			m_GeoPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoPipeline;
			gBufferPassSpec.DebugName = "gBufferPass";
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput(m_UBSCameraData, 0);  // 设置binding=0的ubo
		}
		// GeoAnimPass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBufferAmin";
			framebufferSpec.ClearDepthOnLoad = false;
			framebufferSpec.ClearColorOnLoad = false;
			framebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(0);
			framebufferSpec.ExistingImages[1] = m_GeoFrameBuffer->GetImage(1);
			framebufferSpec.ExistingImages[2] = m_GeoFrameBuffer->GetImage(2);
			framebufferSpec.ExistingImages[3] = m_GeoFrameBuffer->GetImage(3);
			framebufferSpec.ExistingImages[4] = m_GeoFrameBuffer->GetDepthImage();
			m_GeoAnimFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.BoneInfluenceLayout = boneInfluenceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBufferAnim");
			pSpec.TargetFramebuffer = m_GeoAnimFrameBuffer;
			pSpec.DebugName = "GbufferAnimPipeline";
			pSpec.DepthOperator = DepthCompareOperator::LessOrEqual;
			m_GeoAnimPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoAnimPipeline;
			gBufferPassSpec.DebugName = "gBufferAnimPass";
			m_GeoAnimPass = RenderPass::Create(gBufferPassSpec);
			m_GeoAnimPass->SetInput(m_UBSCameraData, 0);  // 设置binding=0的ubo
			m_GeoAnimPass->SetInput(m_SBSBoneTransforms, 1, true);  // 设置binding=1的ubo
		}
	}
	void SceneRender::InitGridPass() {
		FramebufferTextureSpecification gridColorOutputSpec(ImageFormat::RGBA32F);
		FramebufferSpecification gridPassFramebufferSpec;
		gridPassFramebufferSpec.Attachments = { gridColorOutputSpec };
		gridPassFramebufferSpec.DebugName = "Grid";
		gridPassFramebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(2);  // 这张图片谁创建的就指向它，不然会报错
		gridPassFramebufferSpec.Attachments.Attachments[0].LoadOp = AttachmentLoadOp::Load;
		m_GridFrameBuffer = Framebuffer::Create(gridPassFramebufferSpec);
		PipelineSpecification gridPipelineSpec;
		gridPipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("grid");
		gridPipelineSpec.TargetFramebuffer = m_GridFrameBuffer;
		gridPipelineSpec.DepthTest = true;
		gridPipelineSpec.DebugName = "GridPipeline";
		m_GridPipeline = Pipeline::Create(gridPipelineSpec);
		RenderPassSpecification gridPassSpec;
		gridPassSpec.Pipeline = m_GridPipeline;
		gridPassSpec.DebugName = "gridPass";
		m_GridPass = RenderPass::Create(gridPassSpec);
		m_GridPass->SetInput(m_UBSCameraData, 0);  // 设置binding=0的ubo
		m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1, true);
	}
	void SceneRender::UploadMeshAndBoneTransForm() {
		// 收集所有参与渲染的Mesh的变换矩阵存储在m_SubmeshTransformBuffers
		{
			uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

			uint32_t offset = 0;
			for (auto& [key, transformData] : m_MeshTransformMap)
			{
				transformData.TransformOffset = offset * sizeof(TransformVertexData);
				for (const auto& transform : transformData.Transforms)
				{
					m_SubmeshTransformBuffers[frameIndex].Data[offset] = transform;
					offset++;
				}
			}
			// 在这里所有的变换矩阵已经上传到GPU（RT线程）
			m_SubmeshTransformBuffers[frameIndex].Buffer->SetData(m_SubmeshTransformBuffers[frameIndex].Data, offset * sizeof(TransformVertexData));
		}

		// 平铺骨骼信息到一个顶点缓冲区
		uint32_t index = 0;
		for (auto& [key, boneTransformsData] : m_MeshBoneTransformsMap)
		{
			boneTransformsData.BoneTransformsBaseIndex = index;
			for (const auto& boneTransforms : boneTransformsData.BoneTransformsData)
			{
				m_BoneTransformsData[index++] = boneTransforms;
			}
		}

		if (index > 0)
		{
			Ref<SceneRender> instance = this;
			Renderer::Submit([instance, index]() mutable
				{
					instance->m_SBSBoneTransforms->RT_Get()->RT_SetData(instance->m_BoneTransformsData, static_cast<uint32_t>(index * sizeof(BoneTransforms)));
				});
		}
	}
	void SceneRender::HandleResizeRuntime() {
		SetViewprotSize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
		if (NeedResize) {
			// 更新FBO尺寸
			HZ_CORE_WARN("SceneRender::PreRender Resize FBO to {0}x{1}", m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
			m_PreDepthClearFramebuffer->Resize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
			m_PreDepthLoadFramebuffer->Resize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
			m_GeoFrameBuffer->Resize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
			m_GeoAnimFrameBuffer->Resize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight()); // ReSize会重新获取引用的图片，之前的FBO的图片必须立刻创建好
			m_GridFrameBuffer->Resize(m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight());
			NeedResize = false;
		}
	}
	void SceneRender::UploadDescriptorRuntime() {
		m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1);
	}
	void SceneRender::ClearPass(Ref<RenderPass> renderPass, bool explicitClear)
	{
		Renderer::BeginRenderPass(m_CommandBuffer, renderPass, explicitClear);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRender::HandleHZBTexture()
	{
		glm::vec2 viewportSize = { m_SceneData->camera.GetViewportWidth(), m_SceneData->camera.GetViewportHeight() };
		//HZB size must be power of 2's
		const glm::uvec2 numMips = glm::ceil(glm::log2(viewportSize));
		// m_SSROptions.NumDepthMips = glm::max(numMips.x, numMips.y); 
		const glm::uvec2 hzbSize = BIT(numMips);
		m_HierarchicalDepthTexture.Texture->Resize(hzbSize.x, hzbSize.y);
		const glm::vec2 hzbUVFactor = { (glm::vec2)viewportSize / (glm::vec2)hzbSize };
		// m_SSROptions.HZBUvFactor = hzbUVFactor;

		ImageViewSpecification imageViewSpec;
		uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		m_HierarchicalDepthTexture.ImageViews.resize(mipCount);
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
			imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
			imageViewSpec.Mip = mip;
			m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
		}
		CreateHZBPassMaterials();

	}

	void SceneRender::CreateHZBPassMaterials()
	{
		constexpr uint32_t maxMipBatchSize = 4;
		const uint32_t hzbMipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		Ref<Shader> hzbShader = Renderer::GetShaderLibrary()->Get("HZB");
		uint32_t materialIndex = 0;
		m_HZBMaterials.resize(Math::DivideAndRoundUp(hzbMipCount, 4u));
		for (uint32_t startDestMip = 0; startDestMip < hzbMipCount; startDestMip += maxMipBatchSize) {
			Ref<Material> material = Material::Create(hzbShader);
			m_HZBMaterials[materialIndex++] = material;
			if (startDestMip == 0)
				material->SetAlbedoTexture(m_PreDepthPass->GetDepthOutput());
			else
				material->SetAlbedoTexture(m_HierarchicalDepthTexture.Texture);
			const uint32_t endDestMip = glm::min(startDestMip + maxMipBatchSize, hzbMipCount);
			uint32_t destMip;
			for (destMip = startDestMip; destMip < endDestMip; ++destMip)
			{
				uint32_t index = destMip - startDestMip;
				// material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[destMip], index);
			}
			destMip -= startDestMip;
			for (; destMip < maxMipBatchSize; ++destMip)
			{
				// material->Set("o_HZB", m_HierarchicalDepthTexture.ImageViews[hzbMipCount - 1], destMip); // could be white texture?
			}
		}
	}

}
