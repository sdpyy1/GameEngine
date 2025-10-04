#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>

namespace Hazel {
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> HilbertLut;
		Ref<TextureCube> BlackCubeTexture;
		//Ref<Environment> EmptyEnvironment;  ��û��

		std::unordered_map<std::string, std::string> GlobalShaderMacros;
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
		// ������� ͨ��Renderer::submit�ύ�������洢��RenderCommandQueue
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		// ������Ⱦ��
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow()->GetSwapChain().GetImageCount());
		
		// �����������ȾAPI����
		s_RendererAPI = RendererAPI::CreateAPI();

		// ����Shader
		//Ref<Shader> shader = Shader::Create("assets/shaders/vert.spv");

		// ��������
		//uint32_t whiteTextureData = 0xffffffff;
		//TextureSpecification spec;
		//spec.Format = ImageFormat::RGBA;
		//spec.Width = 1;
		//spec.Height = 1;
		//s_Data->WhiteTexture = Texture2D::Create(spec, Buffer1(&whiteTextureData, sizeof(uint32_t)));

		// Ϊ����֡�������������ء���ǰ׼����ȫ���������ݴ�����GPU
		s_RendererAPI->Init();

	}
	void Renderer::RT_BeginFrame()
	{
		s_RendererAPI->RT_BeginFrame();
	}

	void Renderer::RT_EndFrame()
	{
		s_RendererAPI->RT_EndFrame();
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

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
	}

	Ref<Texture2D> Renderer::GetHilbertLut()
	{
		return s_Data->HilbertLut;
	}

	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}


	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}
	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow()->GetSwapChain().GetCurrentBufferIndex();
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
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
		}
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
