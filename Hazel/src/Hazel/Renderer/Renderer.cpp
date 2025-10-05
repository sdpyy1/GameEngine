#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>

namespace Hazel {
	// 这里存储常用渲染资源
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

	// 在这里完成初始资源的加载
	void Renderer::Init()
	{
		// 存储一些提前创建好的渲染资源
		s_Data = new RendererData();
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// 缓存队列 通过Renderer::submit提交的命令都会存储在RenderCommandQueue
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		// 并发渲染数
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow()->GetSwapChain().GetImageCount());
		
		// 创建具体的渲染API对象
		s_RendererAPI = RendererAPI::CreateAPI();

		// 加载Shader
		Shader::DescriptorBinding uboBinding;
		uboBinding.binding = 0; 
		uboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; 
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; 
		uboBinding.count = 1; 
		Shader::ShaderSpecification uboSpec;
		uboSpec.bindings.push_back(uboBinding);
		s_Data->m_ShaderLibrary->LoadCommonShader("test", "assets/shaders/Debug/vert.spv", "assets/shaders/Debug/frag.spv", uboSpec);
		// 加载纹理
		//uint32_t whiteTextureData = 0xffffffff;
		//TextureSpecification spec;
		//spec.Format = ImageFormat::RGBA;
		//spec.Width = 1;
		//spec.Height = 1;
		//s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

		// 为并发帧创建了描述符池、提前准备了全屏顶点数据存入了GPU
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
		s_CommandQueue[GetRenderQueueIndex()]->Execute();  // ???：感觉有问题，这怎么总在执行下一帧的命令（解决：他在执行NextFrame（）时，切换到了下一帧，这里再切换一下又回去了，所以这套逻辑只在一共2个缓冲区的时候没问题）

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
