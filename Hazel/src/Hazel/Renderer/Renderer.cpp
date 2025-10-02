#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/Renderer2D.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>

namespace Hazel {
	// 为了调试。瞎写
	 // 初始化静态成员s_SceneData，并为ViewProjectionMatrix赋予随机值
	Scope<Renderer::SceneData> Renderer::s_SceneData = []() {
		// 创建SceneData实例
		auto sceneData = std::make_unique<Renderer::SceneData>();

		// 生成随机的4x4矩阵（值范围示例：-1000到1000之间）
		sceneData->ViewProjectionMatrix = glm::mat4(
			glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f),
			glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f),
			glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f),
			glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f), glm::linearRand(-1000.0f, 1000.0f)
		);

		return sceneData;
		}();
	static RendererAPI* InitRendererAPI()
	{
		switch (RendererAPI::Current())
		{
			case RendererAPI::Type::Vulkan: return nullptr;
		}
		HZ_CORE_ASSERT(false, "Unknown RendererAPI");
		return nullptr;
	}
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;

		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> HilbertLut;
		Ref<TextureCube> BlackCubeTexture;
		//Ref<Environment> EmptyEnvironment;  还没有

		std::unordered_map<std::string, std::string> GlobalShaderMacros;
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
		// 存储渲染资源
		s_Data = new RendererData();
		// 缓存队列
		s_CommandQueue[0] = new RenderCommandQueue();
		s_CommandQueue[1] = new RenderCommandQueue();
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow().GetSwapChain().GetImageCount());
		
		// 创建具体的渲染API对象
		s_RendererAPI = InitRendererAPI();

		// 加载Shader
		// TODO：这里还设置了Shader 宏，还没有研究   加载初始Shader还没看
		/*Renderer::SetGlobalMacroInShaders("__HZ_REFLECTION_OCCLUSION_METHOD", "0");
		Renderer::SetGlobalMacroInShaders("__HZ_AO_METHOD", std::format("{}", (int)ShaderDef::GetAOMethod(true)));
		Renderer::SetGlobalMacroInShaders("__HZ_GTAO_COMPUTE_BENT_NORMALS", "0");*/
		//s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		//if (!s_Config.ShaderPackPath.empty())
		//// NOTE: some shaders (compute) need to have optimization disabled because of a shaderc internal error
		Ref<Shader> shader = Shader::Create("assets/shaders/vert.spv");

		// 加载纹理
		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		s_Data->WhiteTexture = Texture2D::Create(spec, Buffer1(&whiteTextureData, sizeof(uint32_t)));

	}

	void Renderer::Shutdown()
	{
		Renderer2D::Shutdown();
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}


	void Renderer::OnWindowResize(uint32_t width, uint32_t height)
	{
		RenderCommand::SetViewport(0, 0, width, height);
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

	void Renderer::BeginScene(OrthographicCamera& camera)
	{
		s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::Submit_old(const Ref_old<Shader>& shader, const Ref_old<VertexArray>& vertexArray, const glm::mat4& transform)
	{
		shader->Bind();
		shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
		shader->SetMat4("u_Transform", transform);

		vertexArray->Bind();
		RenderCommand::DrawIndexed(vertexArray);
	}
	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}
	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow().GetSwapChain().GetCurrentBufferIndex();
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
		s_CommandQueue[GetRenderQueueIndex()]->Execute();

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
