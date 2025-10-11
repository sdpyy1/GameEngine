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
		Shader::DescriptorBinding uboBinding;
		uboBinding.binding = 0;
		uboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboBinding.count = 1;
		Shader::DescriptorBinding textureBinding;
		textureBinding.binding = 1;
		textureBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		textureBinding.count = 1;
		gbufferShaderSpec.bindings.push_back(uboBinding);
		gbufferShaderSpec.bindings.push_back(textureBinding);
		//s_Data->m_ShaderLibrary->LoadCommonShader("gBuffer", "assets/shaders/Debug/gBuffervert.spv", "assets/shaders/Debug/gBufferfrag.spv", gbufferShaderSpec);
		s_Data->m_ShaderLibrary->LoadCommonShader("gBuffer", "assets/shaders/Debug/vert.spv", "assets/shaders/Debug/frag.spv", gbufferShaderSpec);


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
	void Renderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer,Ref<VertexBuffer> testVertexBuffer)
	{
		return s_RendererAPI->BindVertData(commandBuffer,testVertexBuffer);

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
