#include "hzpch.h"
#include "SceneRender.h"
#include "Components.h"
#include "Hazel/Asset/AssetManager.h"
#include "Entity.h"
#include <Hazel/Asset/AssetImporter.h>
namespace Hazel {
	SceneRender::SceneRender()
	{
		Init();
	}
	void SceneRender::test()
	{
		// 模型加载
		AssetMetadata metadata;
		//metadata.FilePath = "D:/Hazel-3D-2023/Hazelnut/Resources/Meshes/Default/Capsule.gltf";
		metadata.FilePath = "assets/model/helmet_pbr/DamagedHelmet.gltf";
		//metadata.FilePath = "assets/model/desert-eagle/scene.gltf";
		metadata.Type = AssetType::MeshSource;
		Ref<Asset> Helmet;
		AssetImporter::TryLoadData(metadata, Helmet);
		testVertexBuffer = Helmet.As<MeshSource>()->GetVertexBuffer();
		indexBuffer = Helmet.As<MeshSource>()->GetIndexBuffer();
		AssetHandle a = Helmet.As<MeshSource>()->m_Materials[0];
		AssetManager::GetAsset<MaterialAsset>(a)->Bind();
	}

	void SceneRender::Init()
	{
		test();
		m_CommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		// MVP矩阵的UBO
		m_MVPMatrix = new UniformBufferObject();
		m_MVPUniformBufferSet = UniformBufferSet::Create(sizeof(UniformBufferObject));

		// GeoPass
		{
			FramebufferTextureSpecification gPositionSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gNormalSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gAlbedoSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gMRSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification depthSpec(ImageFormat::DEPTH32F);
			FramebufferAttachmentSpecification attachmentSpec;
			attachmentSpec.Attachments = { ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,ImageFormat::RGBA32F,depthSpec };
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = attachmentSpec;
			framebufferSpec.DebugName = "GBuffer";
			m_GeoFrameBuffer = Framebuffer::Create(framebufferSpec);

			// Pipeline 测试
			PipelineSpecification pSpec;
			pSpec.BackfaceCulling = false;
			pSpec.Layout = Vertex::GetVertexLayout();
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBuffer").As<VulkanShader>();
			pSpec.TargetFramebuffer = m_GeoFrameBuffer;
			pSpec.DebugName = "GbufferPipeline";
			m_GeoPipeline = Pipeline::Create(pSpec);

			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoPipeline;
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput(m_MVPUniformBufferSet, 0);  // 设置binding=0的ubo
		}
	}

	void SceneRender::PreRender(EditorCamera& camera)
	{
		m_CommandBuffer->Begin();
		UpdateMVPMatrix(camera); // 更新MVP矩阵
	}

	void SceneRender::Draw() {
		GeoPass();
	}
	void SceneRender::GeoPass()
	{
		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, true);
		Renderer::BindVertData(m_CommandBuffer, testVertexBuffer);
		Renderer::BindIndexDataAndDraw(m_CommandBuffer, indexBuffer);
		Renderer::EndRenderPass(m_CommandBuffer);
	}

	void SceneRender::EndRender()
	{
		Draw();
		m_CommandBuffer->End();
		m_CommandBuffer->Submit();
	}
	void SceneRender::UpdateMVPMatrix(EditorCamera& camera)
	{
		m_MVPMatrix->model = glm::mat4(1.0);
		m_MVPMatrix->view = camera.GetViewMatrix();
		m_MVPMatrix->proj = camera.GetProjectionMatrix();
		m_MVPMatrix->proj[1][1] *= -1; // Y轴反转
		m_MVPUniformBufferSet->RT_Get()->SetData((void*)m_MVPMatrix, sizeof(UniformBufferObject));
	}
}
