#include "hzpch.h"
#include "SceneRender.h"
#include "Components.h"
#include "Hazel/Asset/AssetManager.h"
#include "Entity.h"
#include <Hazel/Asset/AssetImporter.h>
#include <Hazel/Asset/Model/Mesh.h>
#include <Platform/Vulkan/VulkanRenderCommandBuffer.h>
namespace Hazel {
	SceneRender::SceneRender()
	{
		Init();
	}

	void SceneRender::Init()
	{
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		m_CommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		// MVP矩阵的UBO
		m_CameraData = new CameraData();
		m_CameraDataBufferSet = UniformBufferSet::Create(sizeof(CameraData),"CameraData");

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
		// GeoPass
		{
			FramebufferSpecification framebufferSpec;
			// pos normal albedo mr depth
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::DEPTH32F };
			framebufferSpec.DebugName = "GBuffer";
			m_GeoFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBuffer");
			pSpec.TargetFramebuffer = m_GeoFrameBuffer;
			pSpec.DebugName = "GbufferPipeline";
			m_GeoPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoPipeline;
			gBufferPassSpec.DebugName = "gBufferPass";
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput(m_CameraDataBufferSet, 0);  // 设置binding=0的ubo

		}
		// GeoAnimPass
		{
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::DEPTH32F };
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
			m_GeoAnimPipeline = Pipeline::Create(pSpec);
			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoAnimPipeline;
			gBufferPassSpec.DebugName = "gBufferAnimPass";
			m_GeoAnimPass = RenderPass::Create(gBufferPassSpec);
			m_GeoAnimPass->SetInput(m_CameraDataBufferSet, 0);  // 设置binding=0的ubo
			m_GeoAnimPass->SetInput(m_SBSBoneTransforms, 1, true);  // 设置binding=1的ubo
		}
		// GridPass
		{
			FramebufferTextureSpecification gridColorOutputSpec(ImageFormat::RGBA32F);
			FramebufferSpecification gridPassFramebufferSpec;
			gridPassFramebufferSpec.Attachments = { gridColorOutputSpec };
			gridPassFramebufferSpec.DebugName = "Grid";
			gridPassFramebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(1);  // 这张图片谁创建的就指向它，不然会报错
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
			m_GridPass->SetInput(m_CameraDataBufferSet, 0);  // 设置binding=0的ubo
			m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1,true);

		}

	}

	void SceneRender::PreRender(EditorCamera& camera)
	{	
		// 更新VP矩阵的UniformBuffer
		UpdateVPMatrix(camera);
		if (NeedResize) {
			// 更新FBO尺寸
			HZ_CORE_WARN("SceneRender::PreRender Resize FBO to {0}x{1}", camera.GetViewportWidth(), camera.GetViewportHeight());
			m_GeoFrameBuffer->Resize(camera.GetViewportWidth(), camera.GetViewportHeight());
			m_GeoAnimFrameBuffer->Resize(camera.GetViewportWidth(), camera.GetViewportHeight()); // ReSize会重新获取引用的图片，之前的FBO的图片必须立刻创建好
			m_GridFrameBuffer->Resize(camera.GetViewportWidth(), camera.GetViewportHeight());
			// m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1,true);  // 目前的报错是初始化时Resize引起的，更新深度图片引用如果写在这里必须所有fly都更新，因为他们的资源描述符一直绑定的都是旧的马上会被销毁的图片，但是在这一帧更新其他帧的资源描述符会出报错，因为别的资源描述符可能正在被使用
			NeedResize = false;
		}
		// 只有Resize或图片信息变化时才会触发更新资源描述符
		m_GridPass->SetInput(m_GeoAnimPass->GetDepthOutput(), 1);



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


		m_CommandBuffer->Begin();
	}

	void SceneRender::Draw() {
		GeoPass();
		GeoAnimPass();
		GridPass();
	}
	void SceneRender::GeoAnimPass() 
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoAnimPass, false);
		for (auto& [meshKey, drawCommand] : m_DrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey);
			const auto& boneTransformsData = m_MeshBoneTransformsMap.at(meshKey);
			Renderer::RenderSkeletonMeshWithMaterial(m_CommandBuffer, m_GeoAnimPipeline, drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, boneTransformsData.BoneTransformsBaseIndex, drawCommand.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::GeoPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, false);
		for (auto& [meshKey, drawCommand] : m_StaticMeshDrawList)
		{
			const auto& transformData = m_MeshTransformMap.at(meshKey); 
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline,drawCommand.MeshSource, drawCommand.SubmeshIndex, drawCommand.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, drawCommand.InstanceCount);
		}
		Renderer::EndRenderPass(m_CommandBuffer);
	}
	void SceneRender::GridPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_GridPass, false);
		Renderer::DrawPrueVertex(m_CommandBuffer,6);
		Renderer::EndRenderPass(m_CommandBuffer);

	}
	void SceneRender::EndRender()
	{
		Draw();
		m_CommandBuffer->End();
		m_CommandBuffer->Submit();

		m_DrawList.clear();
		m_StaticMeshDrawList.clear();
		m_MeshTransformMap.clear();
		m_MeshBoneTransformsMap.clear();
	}
	void SceneRender::UpdateVPMatrix(EditorCamera& camera)
	{
		m_CameraData->view = camera.GetViewMatrix();
		m_CameraData->proj = camera.GetProjectionMatrix();
		m_CameraData->proj[1][1] *= -1; // Y轴反转
		m_CameraData->Near = camera.GetNearClip();
		m_CameraData->Far = camera.GetFarClip();
		m_CameraData->Width = camera.GetViewportWidth();
		m_CameraData->Height = camera.GetViewportHeight();
		m_CameraDataBufferSet->Get()->SetData((void*)m_CameraData, sizeof(CameraData));
	}

	// 把Mesh数据解析为DrawList
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

	void SceneRender::SubmitMesh(Ref<MeshSource> meshSource, uint32_t submeshIndex, const glm::mat4& transform, const std::vector<glm::mat4>& boneTransforms)
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
			DrawCommand& drawCommand = m_DrawList[meshKey];
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
		if(width != ViewportWidth || height != ViewportHeight)
		{
			ViewportWidth = width;
			ViewportHeight = height;
			NeedResize = true;
		}
	}

}
