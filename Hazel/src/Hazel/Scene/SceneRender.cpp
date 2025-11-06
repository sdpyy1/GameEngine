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
	void SceneRender::Init()
	{
		m_CommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		InitBuffers();
		preComputeEnv();

		InitDirShadowPass();
		//InitSpotShadowPass();
		InitPreDepthPass();
		InitHZBPass();
		InitGeoPass();
		//InitLightPass();
		InitSceneCompositePass();
		InitGridPass();
	}
	void SceneRender::InitSceneCompositePass()
	{
		FramebufferSpecification sceneCompositeFramebufferSpec;
		sceneCompositeFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
		sceneCompositeFramebufferSpec.DebugName = "FinalColorFrameBuffer";
        sceneCompositeFramebufferSpec.Transfer = true;
		m_SceneCompositeFrameBuffer = Framebuffer::Create(sceneCompositeFramebufferSpec);
		PipelineSpecification sceneCompositePipelineSpec;
		sceneCompositePipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("FinalColor");
		sceneCompositePipelineSpec.TargetFramebuffer = m_SceneCompositeFrameBuffer;
		sceneCompositePipelineSpec.DepthTest = false;
		sceneCompositePipelineSpec.DebugName = "FinalColor";
		m_SceneCompositePipeline = Pipeline::Create(sceneCompositePipelineSpec);
		RenderPassSpecification sceneCompositePassSpec;
		sceneCompositePassSpec.Pipeline = m_SceneCompositePipeline;
		sceneCompositePassSpec.DebugName = "FinalColorPass";
		m_SceneCompositePass = RenderPass::Create(sceneCompositePassSpec);
		m_SceneCompositePass->SetInput(m_GeoFrameBuffer->GetImage(0), 0,true);
	}
	void SceneRender::Draw() {
		ShadowPass();
		// SpotShadowPass();
		PreDepthPass();
		HZBComputePass();
		GeoPass();
		//LightPass();
		SceneCompositePass();
		GridPass();
	}
	void SceneRender::PreRender(SceneInfo sceneData)
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		// 接收场景数据
		m_SceneDataFromScene = &sceneData;
		// 处理Resize
		HandleResizeRuntime();
		// 更新各种资源信息
		UploadCameraData(); // 摄像机数据
		UploadMeshAndBoneTransForm(); // 模型变换和骨骼变换矩阵
		UploadCSMShadowData(); // 级联阴影数据
		UploadSpotShadowData(); // 聚光阴影 TODO：待完成
		UploadRenderSettingData();  // 渲染设置数据
		uploadSceneData(); // 场景数据
		isFirstFrame = false;
	}
	void SceneRender::EndRender()
	{
		m_CommandBuffer->Begin();
		Draw();
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
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, drawCommand.InstanceCount);
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
		float nearOffset = -250.f; // 光空间需要往后移动一些（其实最好是通过计算场景内最远的物体在哪里，然后后移Near平面，不然无法处理光照遮挡了，但不在视锥内的物体）
		float farOffset = 0.f;
		glm::mat4 viewProjection = sceneCamera.GetViewProjection();
		float CascadeSplitLambda = 0.9f;
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
	
		// Calculate orthographic projection matrix for each cascade
		float lastSplitDist = 0.0;
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++)
		{
			float splitDist = cascadeSplits[i];

			glm::vec3 frustumCorners[8] = {
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
			// frustumCorners设置成一个级联的8个顶点位置
			for (uint32_t i = 0; i < 4; i++)
			{
				glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
				frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
				frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
			}

			// 计算级联的中心坐标（8个顶点的平均值）
			glm::vec3 frustumCenter = glm::vec3(0.0f);
			for (uint32_t i = 0; i < 8; i++)
				frustumCenter += frustumCorners[i];

			frustumCenter /= 8.0f;

			// 计算级联的半径（中心点到各个角点距离的最大值）
			float radius = 0.0f;
			for (uint32_t i = 0; i < 8; i++)
			{
				float distance = glm::length(frustumCorners[i] - frustumCenter);
				radius = glm::max(radius, distance);
			}
			// 向上取整到最近的 1/16 整数倍(??)
			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDirection * radius, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f)); // 最终顶点也是用这个view来转移到光空间，所以Up方向不影响
			glm::mat4 lightOrthoMatrix = glm::ortho(-radius, radius, -radius, radius, 0.0f + nearOffset, radius * 2 + farOffset); // TODO:范围设计还需要更加精细化的设计，offset目前设计很大才能包括最后一个级联

			// Offset to texel space to avoid shimmering (from https://stackoverflow.com/questions/33499053/cascaded-shadow-map-shimmering)
			glm::mat4 shadowMatrix = lightOrthoMatrix * lightViewMatrix;
			float ShadowMapResolution = (float)m_DirectionalShadowMapPass[0]->GetSpecification().Pipeline->GetSpecification().TargetFramebuffer->GetWidth();

			glm::vec4 shadowOrigin = (shadowMatrix * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) * ShadowMapResolution / 2.0f;
			glm::vec4 roundedOrigin = glm::round(shadowOrigin);
			glm::vec4 roundOffset = roundedOrigin - shadowOrigin;
			roundOffset = roundOffset * 2.0f / ShadowMapResolution;
			roundOffset.z = 0.0f;
			roundOffset.w = 0.0f;

			lightOrthoMatrix[3] += roundOffset;  // TODO:这个优化还没看懂roundOffset相关的

			// Store split distance and matrix in cascade
			cascades[i].SplitDepth = (nearClip + splitDist * clipRange);
			cascades[i].ViewProj = shadowMatrix;
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
		const std::vector<SpotLight>& spotLightsVec = m_SceneDataFromScene->SceneLightEnvironment.SpotLights;
		// TODO:待完成
	}
	void SceneRender::SpotShadowPass()
	{
		//throw std::logic_error("The method or operation is not implemented.");
	}
	void SceneRender::UploadCSMShadowData() {
        uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		if (m_SceneDataFromScene->SceneLightEnvironment.DirectionalLights[0].Intensity == 0) return;
		CascadeData cascades[4];
		CalculateCascades(cascades, m_SceneDataFromScene->camera, m_SceneDataFromScene->SceneLightEnvironment.DirectionalLights[0].Direction);
		for (int i = 0; i < NumShadowCascades; i++)
		{
			CascadeSplits[i] = cascades[i].SplitDepth;
			m_ViewProjToLigthData[frameIndex].ViewProjection[i] = cascades[i].ViewProj;
		}
		m_UBSViewProjToLight->Get()->SetData(&m_ViewProjToLigthData[frameIndex], sizeof(UBViewProjToLight));
	}
	void SceneRender::ShadowPass() {
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		auto& directionalLights = m_SceneDataFromScene->SceneLightEnvironment.DirectionalLights;
		if (directionalLights[0].Intensity == 0.0f)
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
		m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1);
		Renderer::BeginRenderPass(m_CommandBuffer, m_GridPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 6);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::SceneCompositePass()
	{
		m_SceneCompositePass->SetInput(m_GeoFrameBuffer->GetImage(0), 0);
		Renderer::BeginRenderPass(m_CommandBuffer, m_SceneCompositePass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 3);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::UploadCameraData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_CameraData[frameIndex].view = m_SceneDataFromScene->camera.GetViewMatrix();
		m_CameraData[frameIndex].proj = m_SceneDataFromScene->camera.GetProjectionMatrix();
		m_CameraData[frameIndex].proj[1][1] *= -1; // Y轴反转
		m_CameraData[frameIndex].viewproj = m_CameraData[frameIndex].proj * m_CameraData[frameIndex].view;
		m_CameraData[frameIndex].Near = m_SceneDataFromScene->camera.GetNearClip();
		m_CameraData[frameIndex].Far = m_SceneDataFromScene->camera.GetFarClip();
		m_CameraData[frameIndex].Width = m_SceneDataFromScene->camera.GetViewportWidth();
		m_CameraData[frameIndex].Height = m_SceneDataFromScene->camera.GetViewportHeight();
		m_CameraData[frameIndex].Position = m_SceneDataFromScene->camera.GetPosition();
		m_UBSCameraData->Get()->SetData(&m_CameraData[frameIndex], sizeof(CameraData));
	}
	void SceneRender::SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform) {
		const auto& submeshData = meshSource->GetSubmeshes();
		for (uint32_t submeshIndex = 0; submeshIndex < submeshData.size(); submeshIndex++) {
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
		if (width != m_ViewportWidth || height != m_ViewportHeight)
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
		m_CameraData.resize(framesInFlight);
		m_UBSCameraData = UniformBufferSet::Create(sizeof(CameraData), "CameraData");

		m_ViewProjToLigthData.resize(framesInFlight);
		m_UBSViewProjToLight = UniformBufferSet::Create(sizeof(UBViewProjToLight), "Shadow");

		m_RenderSettingData.resize(framesInFlight);
        m_UBSRenderSetting = UniformBufferSet::Create(sizeof(RenderSettingData), "RenderSettingData");

		m_SceneDataForShader.resize(framesInFlight);
        m_UBSSceneDataForShader = UniformBufferSet::Create(sizeof(SceneDataForShader), "SceneDataForShader");
		
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
		// 一张多Layer深度图用于不同的阴影级
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
		shadowMapFramebufferSpec.DebugName = "DirShadowMap";
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
		{
			shadowMapRenderPassSpec.DebugName = shadowMapFramebufferSpec.DebugName;

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
			m_DirectionalShadowMapPass[i]->SetInput(m_UBSViewProjToLight, 0);

			shadowMapRenderPassSpec.DebugName = "DirShadowPassAnim";
			shadowMapRenderPassSpec.Pipeline = m_ShadowPassPipelinesAnim[i];
			m_DirectionalShadowMapAnimPass[i] = RenderPass::Create(shadowMapRenderPassSpec);
			m_DirectionalShadowMapAnimPass[i]->SetInput(m_UBSViewProjToLight, 0);
			m_DirectionalShadowMapAnimPass[i]->SetInput(m_SBSBoneTransforms, 1, true); // TODO:注意绑定点
		}
	}
	void SceneRender::InitGeoPass() {
		{
			FramebufferSpecification framebufferSpec;
			// pos normal albedo mr depth
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBuffer";
			framebufferSpec.ExistingImages[2] = m_PreDepthLoadFramebuffer->GetDepthImage();
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
			m_GeoPass->SetInput(m_UBSCameraData, 0);
			m_GeoPass->SetInput(m_UBSViewProjToLight, 2);
			m_GeoPass->SetInput(m_UBSRenderSetting, 3);
			m_GeoPass->SetInput(m_UBSSceneDataForShader, 4);
			m_GeoPass->SetInput(m_ShadowPassPipelines[0]->GetSpecification().TargetFramebuffer->GetDepthImage(), 5,true);
			m_GeoPass->SetInput(m_EnvPreFilterMap, 6);
			m_GeoPass->SetInput(m_EnvIrradianceMap, 7);
			m_GeoPass->SetInput(m_EnvLut, 8);

		}
		// GeoAnimPass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::Depth };
			framebufferSpec.DebugName = "GBufferAmin";
			framebufferSpec.ClearDepthOnLoad = false;
			framebufferSpec.ClearColorOnLoad = false;
			framebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(0);
			framebufferSpec.ExistingImages[1] = m_GeoFrameBuffer->GetImage(1);
			framebufferSpec.ExistingImages[2] = m_GeoFrameBuffer->GetDepthImage();
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
			m_GeoAnimPass->SetInput(m_UBSCameraData, 0);
			m_GeoAnimPass->SetInput(m_UBSViewProjToLight, 2);
			m_GeoAnimPass->SetInput(m_UBSRenderSetting, 3);
			m_GeoAnimPass->SetInput(m_UBSSceneDataForShader, 4);
			m_GeoAnimPass->SetInput(m_ShadowPassPipelines[0]->GetSpecification().TargetFramebuffer->GetDepthImage(), 5, true);
			m_GeoAnimPass->SetInput(m_EnvPreFilterMap, 6);
			m_GeoAnimPass->SetInput(m_EnvIrradianceMap, 7);
			m_GeoAnimPass->SetInput(m_EnvLut, 8);
		}
	}
	void SceneRender::InitGridPass() {
		FramebufferTextureSpecification gridColorOutputSpec(ImageFormat::RGBA32F);
		FramebufferSpecification gridPassFramebufferSpec;
		gridPassFramebufferSpec.Attachments = { gridColorOutputSpec };
		gridPassFramebufferSpec.DebugName = "Grid";
		gridPassFramebufferSpec.ExistingImages[0] = m_SceneCompositeFrameBuffer->GetImage(0); 
		gridPassFramebufferSpec.Attachments.Attachments[0].LoadOp = AttachmentLoadOp::Load;
		m_GridFrameBuffer = Framebuffer::Create(gridPassFramebufferSpec);
		PipelineSpecification gridPipelineSpec;
		gridPipelineSpec.Shader = Renderer::GetShaderLibrary()->Get("grid");
		gridPipelineSpec.TargetFramebuffer = m_GridFrameBuffer;
		gridPipelineSpec.DepthTest = false;
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
		SetViewprotSize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
		if (NeedResize) {
			// 更新FBO尺寸（需要按顺序Resize）
			HZ_CORE_WARN("SceneRender::PreRender Resize FBO to {0}x{1}", m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			m_PreDepthClearFramebuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			m_PreDepthLoadFramebuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			m_GeoFrameBuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			m_GeoAnimFrameBuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			// m_LightPassFramebuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight()); 
			m_SceneCompositeFrameBuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			m_GridFrameBuffer->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
			HandleHZBResize();
			NeedResize = false;
		}
	}

	void SceneRender::ClearPass(Ref<RenderPass> renderPass, bool explicitClear)
	{
		Renderer::BeginRenderPass(m_CommandBuffer, renderPass, explicitClear);
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::HandleHZBResize()
	{
		m_HierarchicalDepthTexture.Texture->Resize(m_SceneDataFromScene->camera.GetViewportWidth(), m_SceneDataFromScene->camera.GetViewportHeight());
		uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		m_HierarchicalDepthTexture.ImageViews.resize(m_HierarchicalDepthTexture.Texture->GetMipLevelCount());
		ImageViewSpecification imageViewSpec;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
			imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
			imageViewSpec.Mip = mip;
			m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
		}
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
		spec.Storage = true;
		m_HierarchicalDepthTexture.Texture = Texture2D::Create(spec); // 创建了一张纹理用于存储最终的HZB
		uint32_t mipCount = m_HierarchicalDepthTexture.Texture->GetMipLevelCount();
		m_HierarchicalDepthTexture.ImageViews.resize(m_HierarchicalDepthTexture.Texture->GetMipLevelCount());
		ImageViewSpecification imageViewSpec;
		for (uint32_t mip = 0; mip < mipCount; mip++)
		{
			imageViewSpec.DebugName = fmt::format("HierarchicalDepthTexture-{}", mip);
			imageViewSpec.Image = m_HierarchicalDepthTexture.Texture->GetImage();
			imageViewSpec.Mip = mip;
			m_HierarchicalDepthTexture.ImageViews[mip] = ImageView::Create(imageViewSpec);
		}
		Ref<Shader> shader = Renderer::GetShaderLibrary()->Get("HZB");
		ComputePassSpecification hdPassSpec;
		hdPassSpec.DebugName = "HierarchicalDepth";
		hdPassSpec.Pipeline = PipelineCompute::Create(shader);
		m_HierarchicalDepthPass = ComputePass::Create(hdPassSpec);
	}
	void SceneRender::HZBComputePass()
	{
		uint32_t inputWidth = m_PreDepthLoadFramebuffer->GetDepthImage()->GetWidth();
		uint32_t inputHeight = m_PreDepthLoadFramebuffer->GetDepthImage()->GetHeight();

		// 工作组数量（按第0级尺寸）
		const uint32_t localSize = 8;
		uint32_t groupCountX = (inputWidth + localSize - 1) / localSize;
		uint32_t groupCountY = (inputHeight + localSize - 1) / localSize;

		// 绑定资源：输入深度图（采样器）+ HZB数组（存储图像）
		Renderer::BeginComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
		m_HierarchicalDepthPass->SetInput(m_PreDepthLoadFramebuffer->GetDepthImage(), 0);
		uint32_t mipLevels = 0;
		for (mipLevels = 0; mipLevels < m_HierarchicalDepthTexture.ImageViews.size(); mipLevels++) {
			m_HierarchicalDepthPass->SetInputOneLayer(m_HierarchicalDepthTexture.ImageViews[mipLevels], 1, mipLevels);
		}

		const Buffer mip(&mipLevels, sizeof(uint32_t));

		Renderer::DispatchCompute(m_CommandBuffer, m_HierarchicalDepthPass, nullptr, glm::uvec3(groupCountX, groupCountY, 1), mip);
		Renderer::EndComputePass(m_CommandBuffer, m_HierarchicalDepthPass);
	}
	void SceneRender::InitEnvNeed()
	{
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = Renderer::GetConfig().IrradianceMapSize;
		m_EnvEquirect = Texture2D::Create(TextureSpecification(), std::filesystem::path("Assets/HDR/6.hdr"));
		HZ_CORE_ASSERT(m_EnvEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");
		TextureSpecification cubemapSpec;
		cubemapSpec.Format = ImageFormat::RGBA32F;
		cubemapSpec.Width = cubemapSize;
		cubemapSpec.Height = cubemapSize;
		cubemapSpec.Storage = true;
		cubemapSpec.GenerateMips = false;
		m_EnvCubeMap = TextureCube::Create(cubemapSpec);
		m_EnvPreFilterMap = TextureCube::Create(cubemapSpec);

		TextureSpecification IrradianceMapSpec;
		IrradianceMapSpec.Format = ImageFormat::RGBA32F;
		IrradianceMapSpec.Width = irradianceMapSize;
		IrradianceMapSpec.Height = irradianceMapSize;
		IrradianceMapSpec.Storage = true;
		IrradianceMapSpec.GenerateMips = false;
		m_EnvIrradianceMap = TextureCube::Create(IrradianceMapSpec);

		// HDR->Cubemap Pass
		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		ComputePassSpecification equirectangularSpec;
		equirectangularSpec.DebugName = "EquirectangularToCubeMap";
		equirectangularSpec.Pipeline = PipelineCompute::Create(equirectangularConversionShader);
		equirectangularSpec.moreDescriptors = 1;
		m_EquirectangularPass = ComputePass::Create(equirectangularSpec);

		// irradianceMap Pass
		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		ComputePassSpecification environmentIrradianceSpec;
		environmentIrradianceSpec.DebugName = "EnvironmentIrradiance";
		environmentIrradianceSpec.Pipeline = PipelineCompute::Create(environmentIrradianceShader);
		m_EnvironmentIrradiancePass = ComputePass::Create(environmentIrradianceSpec);

		// PreFilterCubeMap Pass
		Ref<Shader> preFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		ComputePassSpecification preFilterSpec;
		preFilterSpec.DebugName = "EnvironmentMipFilter";
		preFilterSpec.Pipeline = PipelineCompute::Create(preFilterShader);
		preFilterSpec.moreDescriptors = m_EnvCubeMap->GetMipLevelCount();
		m_EnvironmentPreFilterPass = ComputePass::Create(preFilterSpec);

		m_EnvLut = Renderer::GetBRDFLutTexture();
	}
	void SceneRender::preComputeEnv()
	{
		InitEnvNeed();
		m_CommandBuffer->Begin();
		const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
		const uint32_t irradianceMapSize = Renderer::GetConfig().IrradianceMapSize;

		// HDR转CubemapPass
		Renderer::BeginComputePass(m_CommandBuffer, m_EquirectangularPass);
		m_EquirectangularPass->SetInput(m_EnvEquirect, 1);
		m_EquirectangularPass->SetInput(m_EnvCubeMap, 0, InputType::stoage);
		Renderer::DispatchCompute(m_CommandBuffer, m_EquirectangularPass, nullptr, glm::ivec3(cubemapSize / 32, cubemapSize / 32, 6));
		Renderer::EndComputePass(m_CommandBuffer, m_EquirectangularPass);

		Renderer::Submit([this]() {
			m_EnvCubeMap->GenerateMips(m_CommandBuffer); // DispatchCompute无法自动生成mipmap，所以这里手动生成，在MipFilter计算中会使用
			});
		// PreFilterCubeMap第一层用HDR
		Renderer::BeginComputePass(m_CommandBuffer, m_EquirectangularPass);
		m_EquirectangularPass->SetInput(m_EnvEquirect, 1, 0);
		m_EquirectangularPass->SetInput(m_EnvPreFilterMap, 0, InputType::stoage, 0);
		Renderer::DispatchCompute(m_CommandBuffer, m_EquirectangularPass, nullptr, glm::ivec3(cubemapSize / 32, cubemapSize / 32, 6), 0);
		Renderer::EndComputePass(m_CommandBuffer, m_EquirectangularPass);

		// 环境Irradiance
		Renderer::BeginComputePass(m_CommandBuffer, m_EnvironmentIrradiancePass);
		m_EnvironmentIrradiancePass->SetInput(m_EnvCubeMap, 1, InputType::sampler);
		m_EnvironmentIrradiancePass->SetInput(m_EnvIrradianceMap, 0, InputType::stoage);
		Renderer::DispatchCompute(m_CommandBuffer, m_EnvironmentIrradiancePass, nullptr, glm::ivec3(irradianceMapSize / 32, irradianceMapSize / 32, 6), Buffer(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t)));
		Renderer::Submit([this]() {
			m_EnvIrradianceMap->GenerateMips(m_CommandBuffer);
			});
		Renderer::EndComputePass(m_CommandBuffer, m_EnvironmentIrradiancePass);

		// 环境MipFilter
		Renderer::BeginComputePass(m_CommandBuffer, m_EnvironmentPreFilterPass);
		uint32_t mipCount = m_EnvCubeMap->GetMipLevelCount();
		for (uint32_t i = 1; i < mipCount; i++) {
			m_EnvironmentPreFilterPass->SetInput(m_EnvPreFilterMap, 0, InputType::stoage, i, i);
			m_EnvironmentPreFilterPass->SetInput(m_EnvCubeMap, 1, InputType::sampler, i);
		}
		const float deltaRoughness = 1.0f / glm::max((float)mipCount - 1.0f, 1.0f);
		for (uint32_t i = 1, size = cubemapSize; i < mipCount; i++, size /= 2)
		{
			uint32_t numGroups = glm::max(1u, size / 32);
			float roughness = i * deltaRoughness;
			Renderer::DispatchCompute(m_CommandBuffer, m_EnvironmentPreFilterPass, nullptr, glm::ivec3(numGroups, numGroups, 6), i, { &roughness, sizeof(float) });
		}
		Renderer::EndComputePass(m_CommandBuffer, m_EnvironmentPreFilterPass);
		m_CommandBuffer->End();
		m_CommandBuffer->Submit();
	}
	SceneRender::SceneRender()
	{
		Init();
	}


	void SceneRender::InitLightPass()
	{
		FramebufferSpecification lightPassFramebufferSpec;
		lightPassFramebufferSpec.Attachments = { ImageFormat::RGBA32F };
		lightPassFramebufferSpec.DebugName = "LightPass";
		m_LightPassFramebuffer = Framebuffer::Create(lightPassFramebufferSpec);
		PipelineSpecification pSpec;
		pSpec.Shader = Renderer::GetShaderLibrary()->Get("Lighting");
		pSpec.TargetFramebuffer = m_LightPassFramebuffer;
		pSpec.DebugName = "LightPassPipeline";
		pSpec.DepthTest = false;
		m_LightPassPipeline = Pipeline::Create(pSpec);
		RenderPassSpecification lightPassSpec;
		lightPassSpec.Pipeline = m_LightPassPipeline;
		lightPassSpec.DebugName = "LightPass";
		m_LightPass = RenderPass::Create(lightPassSpec);
		m_LightPass->SetInput(m_EnvIrradianceMap, 4);
		m_LightPass->SetInput(m_EnvPreFilterMap, 5);
		m_LightPass->SetInput(m_EnvLut, 6);
	}
	void SceneRender::LightPass()
	{
		m_LightPass->SetInput(m_GeoFrameBuffer->GetImage(0), 0);
		m_LightPass->SetInput(m_GeoFrameBuffer->GetImage(1), 1);
		m_LightPass->SetInput(m_GeoFrameBuffer->GetImage(2), 2);
		m_LightPass->SetInput(m_GeoFrameBuffer->GetImage(3), 3);

		Renderer::BeginRenderPass(m_CommandBuffer, m_LightPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer, 3);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRender::UploadRenderSettingData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_RenderSettingData[frameIndex].CascadeSplits = CascadeSplits;
		m_RenderSettingData[frameIndex].ShadowType = m_SceneDataFromScene->RenderSettingData.ShadowType;
		m_RenderSettingData[frameIndex].deBugCSM = m_SceneDataFromScene->RenderSettingData.deBugCSM;
		m_UBSRenderSetting->Get()->SetData(&m_RenderSettingData[frameIndex], sizeof(RenderSettingData));
	}

	void SceneRender::uploadSceneData()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();
		m_SceneDataForShader[frameIndex].DirectionalLight = m_SceneDataFromScene->SceneLightEnvironment.DirectionalLights[0];
		m_SceneDataForShader[frameIndex].EnvironmentMapIntensity = 1.f;
		m_UBSSceneDataForShader->Get()->SetData(&m_SceneDataForShader[frameIndex], sizeof(SceneDataForShader));
	}



}
