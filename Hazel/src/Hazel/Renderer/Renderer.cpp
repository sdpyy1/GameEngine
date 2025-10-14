#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <Platform/Vulkan/VulkanImage.h>

namespace Hazel {
	Ref<Texture2D> Renderer::WhiteTexture = nullptr;
	// ����洢������Ⱦ��Դ
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2;
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];
	static RendererAPI* s_RendererAPI = nullptr;

	// ��������ɳ�ʼ��Դ�ļ���
	void Renderer::Init()
	{
		// �洢һЩ��ǰ�����õ���Ⱦ��Դ
		s_Data = new RendererData();
		// Shader����
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// ������� ͨ��Renderer::submit�ύ�������洢��RenderCommandQueue
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		// ������Ⱦ��
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow()->GetSwapChain().GetImageCount());
		// �����������ȾAPI����
		s_RendererAPI = RendererAPI::CreateAPI();
		// Shader���棬��Ҫ����Shader��Դ��������Ϣ��Shader�ᴴ������Դ������Set
		// gBufferShader
		Shader::ShaderSpecification gbufferShaderSpec;
		// 1. ������ɫ���� UBO��ģ��/��ͼ/ͶӰ����ȣ�
		Shader::DescriptorBinding uboBinding;
		uboBinding.binding = 0;                  // ��Ӧ��ɫ���� binding = 0
		uboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // ������ɫ��ʹ��
		uboBinding.count = 1;
		uboBinding.set = 0;
		gbufferShaderSpec.bindings.push_back(uboBinding);
		// 2. Albedo ��ͼ��combined image sampler��
		Shader::DescriptorBinding albedoBinding;
		albedoBinding.binding = 0;               // ��Ӧ��ɫ���� set=0, binding=0��ע��������Ӧ��
		albedoBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  // Ƭ����ɫ��ʹ��
		albedoBinding.count = 1;
		albedoBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(albedoBinding);
		// 3. Normal ��ͼ
		Shader::DescriptorBinding normalBinding;
		normalBinding.binding = 1;               // ��Ӧ��ɫ���� set=0, binding=1
		normalBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		normalBinding.count = 1;
		normalBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(normalBinding);
		// 4. Metalness ��ͼ
		Shader::DescriptorBinding metalnessBinding;
		metalnessBinding.binding = 2;            // ��Ӧ��ɫ���� set=0, binding=2
		metalnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metalnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		metalnessBinding.count = 1;
		metalnessBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(metalnessBinding);
		// 5. Roughness ��ͼ
		Shader::DescriptorBinding roughnessBinding;
		roughnessBinding.binding = 3;            // ��Ӧ��ɫ���� set=0, binding=3
		roughnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		roughnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		roughnessBinding.count = 1;
		roughnessBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(roughnessBinding);

		// ������ɫ����ȷ�� vert.spv �� frag.spv ��������ƥ�䣩
		s_Data->m_ShaderLibrary->LoadCommonShader(
			"gBuffer",
			"assets/shaders/Debug/vert.spv",
			"assets/shaders/Debug/frag.spv",
			gbufferShaderSpec
		);

		// ��������
		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));
		// Ϊ����֡�������������ء���ǰ׼����ȫ���������ݴ�����GPU
		s_RendererAPI->Init();

	}
	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}

	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	void Renderer::Shutdown()
	{
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}


	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}



	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		return s_RendererAPI->BeginRenderPass(renderCommandBuffer,renderPass,explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		return s_RendererAPI->EndRenderPass(renderCommandBuffer);

	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}
	//void Renderer::DrawStaticMesh(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> vertexBuffer, Ref<MeshSource> meshSource, uint32_t subMeshIndex) {

	//}

	void Renderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer,Ref<VertexBuffer> testVertexBuffer)
	{
		return s_RendererAPI->BindVertData(commandBuffer,testVertexBuffer);

	}
	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer,Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount)
	{
		return s_RendererAPI->RenderStaticMeshWithMaterial(commandBuffer, pipeline,meshSource, submeshIndex, material, transformBuffer, transformOffset, instanceCount);
	}
	void Renderer::BindIndexDataAndDraw(Ref<RenderCommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer)
	{
		return s_RendererAPI->BindIndexDataAndDraw(commandBuffer,indexBuffer);

	}
	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return WhiteTexture;
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetCurrentFrameIndex();
	}

	void Renderer::RenderThreadFunc(RenderThread* renderThread)
	{
		while (renderThread->IsRunning())
		{
			WaitAndRender(renderThread);
		}
	}
	void Renderer::WaitAndRender(RenderThread* renderThread)
	{
		// Wait for kick, then set render thread to busy  
		{
			HZ_PROFILE_SCOPE("Wait");
			// ��Ⱦ�߳�ѭ���ȴ�Kick�źţ��յ��źź�����ΪBusy�źŲ���ʼ����
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
		}
		// �������ǰѻ�������ȫ��ִ��
		s_CommandQueue[GetRenderQueueIndex()]->Execute();  // ???���о������⣬����ô����ִ����һ֡��������������ִ��NextFrame����ʱ���л�������һ֡���������л�һ���ֻ�ȥ�ˣ����������߼�ֻ��һ��2����������ʱ��û���⣩

		// Rendering has completed, set state to idle
		renderThread->Set(RenderThread::State::Idle);
	}
	uint32_t Renderer::GetRenderQueueIndex()
	{
		return (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}
	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}
	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}
}
