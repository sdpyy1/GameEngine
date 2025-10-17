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
		// MVP�����UBO
		m_CameraData = new UniformBufferObject();
		m_VPUniformBufferSet = UniformBufferSet::Create(sizeof(UniformBufferObject));

		// ��һ���ܴ�Ķ��㻺�������洢���еı任����
		const size_t TransformBufferCount = 10 * 1024; // 10240 transforms
		m_SubmeshTransformBuffers.resize(framesInFlight);
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_SubmeshTransformBuffers[i].Buffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount);
			m_SubmeshTransformBuffers[i].Data = new TransformVertexData[TransformBufferCount];
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
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput(m_VPUniformBufferSet, 0);  // ����binding=0��ubo
		}

		// GridPass
		{
			FramebufferTextureSpecification gridColorOutputSpec(ImageFormat::RGBA32F);
			FramebufferSpecification gridPassFramebufferSpec;
			gridPassFramebufferSpec.Attachments = { gridColorOutputSpec };
			gridPassFramebufferSpec.DebugName = "Grid";
			gridPassFramebufferSpec.ExistingImages[0] = m_GeoFrameBuffer->GetImage(2); 
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
			m_GridPass = RenderPass::Create(gridPassSpec);
			m_GridPass->SetInput(m_VPUniformBufferSet, 0);  // ����binding=0��ubo
		}

	}

	void SceneRender::PreRender(EditorCamera& camera)
	{	
		if (NeedResize) {
			// ����FBO�ߴ�
			m_GeoFrameBuffer->Resize(camera.GetViewportWidth(), camera.GetViewportHeight());
			m_GridFrameBuffer->Resize(camera.GetViewportWidth(), camera.GetViewportHeight());
			NeedResize = false;
		}
		m_GridPass->SetInput(m_GeoPass->GetDepthOutput(), 1);  // ���ֻ�����FBO�ߴ�仯���仯�����룬����ÿ֡����

		// ����VP�����UniformBuffer
		UpdateVPMatrix(camera); 
		// �ռ����в�����Ⱦ��Mesh�ı任����洢��m_SubmeshTransformBuffers
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
			// ���������еı任�����Ѿ��ϴ���GPU��RT�̣߳�
			m_SubmeshTransformBuffers[frameIndex].Buffer->SetData(m_SubmeshTransformBuffers[frameIndex].Data, offset * sizeof(TransformVertexData)); 
		}

		m_CommandBuffer->Begin();
	}

	void SceneRender::Draw() {
		GeoPass();
		GridPass();
	}
	void SceneRender::GeoPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, true);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			// ��ȡ����һ��SubMesh�ı任������Ϣ���������ʵ����
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline,dc.MeshSource, dc.SubmeshIndex, dc.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
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

		m_StaticMeshDrawList.clear();
		m_MeshTransformMap.clear();

	}
	void SceneRender::UpdateVPMatrix(EditorCamera& camera)
	{
		m_CameraData->view = camera.GetViewMatrix();
		m_CameraData->proj = camera.GetProjectionMatrix();
		m_CameraData->proj[1][1] *= -1; // Y�ᷴת
		m_CameraData->width = camera.GetViewportWidth();
		m_CameraData->height = camera.GetViewportHeight();
		m_VPUniformBufferSet->RT_Get()->SetData((void*)m_CameraData, sizeof(UniformBufferObject));
	}

	// ��Mesh���ݽ���ΪDrawList
	void SceneRender::SubmitStaticMesh(Ref<MeshSource> meshSource, const glm::mat4& transform) {
		const auto& submeshData = meshSource->GetSubmeshes();
		for(uint32_t submeshIndex = 0; submeshIndex <submeshData.size(); submeshIndex++){
			glm::mat4 submeshTransform = transform * submeshData[submeshIndex].Transform;  // subMesh��ȫ�ֱ任
			AssetHandle materialHandle = meshSource->GetMaterialHandle(submeshData[submeshIndex].MaterialIndex);
			Ref<MaterialAsset> material = AssetManager::GetAsset<MaterialAsset>(materialHandle);// subMesh�Ĳ�������;
			HZ_CORE_VERIFY(material);
			MeshKey meshKey = { meshSource->Handle, materialHandle, submeshIndex, false };
			// ����任����
			auto& transformStorage = m_MeshTransformMap[meshKey].Transforms.emplace_back(); // ����ÿһ��MeshKey�����洢���Transforms���󣬱�ʾ���ʵ��
			transformStorage.MRow[0] = { submeshTransform[0][0], submeshTransform[1][0], submeshTransform[2][0], submeshTransform[3][0] };
			transformStorage.MRow[1] = { submeshTransform[0][1], submeshTransform[1][1], submeshTransform[2][1], submeshTransform[3][1] };
			transformStorage.MRow[2] = { submeshTransform[0][2], submeshTransform[1][2], submeshTransform[2][2], submeshTransform[3][2] };
			// �����������
			auto& destDrawList = m_StaticMeshDrawList;
			auto& dc = destDrawList[meshKey];
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialAsset = material;
			dc.InstanceCount++;
		}
	};


	void SceneRender::SetViewprotSize(float width, float height) {
		if(width != ViewportWidth || height != ViewportHeight)
		{
			ViewportWidth = width;
			ViewportHeight = height;
			NeedResize = true;
		}
	}

}
