#include "hzpch.h"
#include "SceneRender.h"
#include "Components.h"
#include "Hazel/Asset/AssetManager.h"
#include "Entity.h"
#include <Hazel/Asset/AssetImporter.h>
#include <Hazel/Asset/Model/Mesh.h>
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
		m_VPMatrix = new UniformBufferObject();
		m_VPUniformBufferSet = UniformBufferSet::Create(sizeof(UniformBufferObject));
		const size_t TransformBufferCount = 10 * 1024; // 10240 transforms
		m_SubmeshTransformBuffers.resize(framesInFlight); // 用一个很大的顶点缓冲区来存储所有的变换矩阵
		for (uint32_t i = 0; i < framesInFlight; i++)
		{
			m_SubmeshTransformBuffers[i].Buffer = VertexBuffer::Create(sizeof(TransformVertexData) * TransformBufferCount);
			m_SubmeshTransformBuffers[i].Data = new TransformVertexData[TransformBufferCount];
		}

		// GeoPass
		{
			FramebufferTextureSpecification gPositionSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gNormalSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gAlbedoSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification gMRSpec(ImageFormat::RGBA32F);
			FramebufferTextureSpecification depthSpec(ImageFormat::DEPTH32F);
			FramebufferAttachmentSpecification attachmentSpec;
			attachmentSpec.Attachments = { gPositionSpec,gNormalSpec,gAlbedoSpec,gMRSpec,depthSpec };
			FramebufferSpecification framebufferSpec;
			framebufferSpec.Attachments = attachmentSpec;
			framebufferSpec.DebugName = "GBuffer";
			m_GeoFrameBuffer = Framebuffer::Create(framebufferSpec);
			PipelineSpecification pSpec;
			pSpec.BackfaceCulling = false;
			pSpec.Layout = vertexLayout;
			pSpec.InstanceLayout = instanceLayout;
			pSpec.Shader = Renderer::GetShaderLibrary()->Get("gBuffer");
			pSpec.TargetFramebuffer = m_GeoFrameBuffer;
			pSpec.DebugName = "GbufferPipeline";
			m_GeoPipeline = Pipeline::Create(pSpec);

			RenderPassSpecification gBufferPassSpec;
			gBufferPassSpec.Pipeline = m_GeoPipeline;
			m_GeoPass = RenderPass::Create(gBufferPassSpec);
			m_GeoPass->SetInput(m_VPUniformBufferSet, 0);  // 设置binding=0的ubo
		}
	}

	void SceneRender::PreRender(EditorCamera& camera)
	{
		UpdateVPMatrix(camera); // 更新VP矩阵的UniformBuffer

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

		m_CommandBuffer->Begin();
	}

	void SceneRender::Draw() {
		GeoPass();
	}
	void SceneRender::GeoPass()
	{
		uint32_t frameIndex = Renderer::GetCurrentFrameIndex();

		Renderer::BeginRenderPass(m_CommandBuffer, m_GeoPass, true);
		for (auto& [mk, dc] : m_StaticMeshDrawList)
		{
			// 获取具体一个SubMesh的变换矩阵信息（包含多个实例）
			const auto& transformData = m_MeshTransformMap.at(mk);
			Renderer::RenderStaticMeshWithMaterial(m_CommandBuffer, m_GeoPipeline,dc.MeshSource, dc.SubmeshIndex, dc.MaterialAsset->GetMaterial(), m_SubmeshTransformBuffers[frameIndex].Buffer, transformData.TransformOffset, dc.InstanceCount);
		}		
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
		m_VPMatrix->view = camera.GetViewMatrix();
		m_VPMatrix->proj = camera.GetProjectionMatrix();
		m_VPMatrix->proj[1][1] *= -1; // Y轴反转
		m_VPUniformBufferSet->RT_Get()->SetData((void*)m_VPMatrix, sizeof(UniformBufferObject));
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
			auto& destDrawList = m_StaticMeshDrawList;
			auto& dc = destDrawList[meshKey];
			dc.MeshSource = meshSource;
			dc.SubmeshIndex = submeshIndex;
			dc.MaterialAsset = material;
			dc.InstanceCount++;
		}
	};
}
