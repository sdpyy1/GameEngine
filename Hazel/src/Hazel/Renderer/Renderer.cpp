#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <Platform/Vulkan/VulkanImage.h>

namespace Hazel {
	Ref<Texture2D> Renderer::WhiteTexture = nullptr;
	// 这里存储常用渲染资源
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2; // 目前代码只支持=2
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];
	static RendererAPI* s_RendererAPI = nullptr;

	// 在这里完成初始资源的加载
	void Renderer::Init()
	{
		// 存储一些提前创建好的渲染资源
		s_Data = new RendererData();
		// Shader缓存
		s_Data->m_ShaderLibrary = Ref<ShaderLibrary>::Create();

		// 缓存队列 通过Renderer::submit提交的命令都会存储在RenderCommandQueue
		for (int i = 0; i < s_RenderCommandQueueCount; i++) {
			s_CommandQueue[i] = new RenderCommandQueue();
		}
		// 并发渲染数
		s_Config.FramesInFlight = glm::min<uint32_t>(s_Config.FramesInFlight, Application::Get().GetWindow()->GetSwapChain().GetImageCount());
		// 创建具体的渲染API对象
		s_RendererAPI = RendererAPI::CreateAPI();
		// Shader缓存，需要传给Shader资源描述符信息，Shader会创建好资源描述符Set
		// gBufferShader
		Shader::ShaderSpecification gbufferShaderSpec;
		// 1. 顶点着色器的 UBO（模型/视图/投影矩阵等）
		Shader::DescriptorBinding uboBinding;
		uboBinding.binding = 0;                  // 对应着色器中 binding = 0
		uboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;  // 顶点着色器使用
		uboBinding.count = 1;
		uboBinding.set = 0;
		gbufferShaderSpec.bindings.push_back(uboBinding);
		// 2. Albedo 贴图（combined image sampler）
		Shader::DescriptorBinding albedoBinding;
		albedoBinding.binding = 0;               // 对应着色器中 set=0, binding=0（注意索引对应）
		albedoBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;  // 片段着色器使用
		albedoBinding.count = 1;
		albedoBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(albedoBinding);
		// 3. Normal 贴图
		Shader::DescriptorBinding normalBinding;
		normalBinding.binding = 1;               // 对应着色器中 set=0, binding=1
		normalBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		normalBinding.count = 1;
		normalBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(normalBinding);
		// 4. Metalness 贴图
		Shader::DescriptorBinding metalnessBinding;
		metalnessBinding.binding = 2;            // 对应着色器中 set=0, binding=2
		metalnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metalnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		metalnessBinding.count = 1;
		metalnessBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(metalnessBinding);
		// 5. Roughness 贴图
		Shader::DescriptorBinding roughnessBinding;
		roughnessBinding.binding = 3;            // 对应着色器中 set=0, binding=3
		roughnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		roughnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		roughnessBinding.count = 1;
		roughnessBinding.set = 1;
		gbufferShaderSpec.bindings.push_back(roughnessBinding);

		// 加载着色器（确保 vert.spv 和 frag.spv 与上述绑定匹配）
		s_Data->m_ShaderLibrary->LoadCommonShader("gBuffer",gbufferShaderSpec);
		Shader::PushConstantRange pr;
		pr.shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
		pr.offset = 0;
		pr.size = 4;
		gbufferShaderSpec.pushConstantRanges = { pr };
		Shader::DescriptorBinding boneTrasnfromBinding;
		boneTrasnfromBinding.binding = 1;            // 对应着色器中 set=0, binding=1
		boneTrasnfromBinding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		boneTrasnfromBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		boneTrasnfromBinding.count = 1;
		boneTrasnfromBinding.set = 0;
		gbufferShaderSpec.bindings.push_back(boneTrasnfromBinding);
		s_Data->m_ShaderLibrary->LoadCommonShader("gBufferAnim", gbufferShaderSpec);

		// 深度贴图Binding
		Shader::DescriptorBinding griduboBinding;
		griduboBinding.binding = 0;                  // 对应着色器中 binding = 0
		griduboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		griduboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;  // 顶点着色器使用
		griduboBinding.count = 1;
		griduboBinding.set = 0;
		gbufferShaderSpec.bindings.push_back(griduboBinding);
		Shader::DescriptorBinding depthBinding;
		depthBinding.binding = 1;            // 对应着色器中 set=0, binding=3
		depthBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		depthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		depthBinding.count = 1;
		depthBinding.set = 0;
		Shader::ShaderSpecification gridShaderSpec;
		gridShaderSpec.bindings.push_back(griduboBinding);
		gridShaderSpec.bindings.push_back(depthBinding);
		s_Data->m_ShaderLibrary->LoadCommonShader("grid", gridShaderSpec);



		// 加载纹理
		uint32_t whiteTextureData = 0xffffffff;
		TextureSpecification spec;
		spec.Format = ImageFormat::RGBA;
		spec.Width = 1;
		spec.Height = 1;
		WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));
		// 为并发帧创建了描述符池、提前准备了全屏顶点数据存入了GPU
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
	void Renderer::RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount)
	{
		s_RendererAPI->RenderSkeletonMeshWithMaterial(renderCommandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, boneTransformsOffset, instanceCount);

	}
	void Renderer::DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count)
	{
		return s_RendererAPI->DrawPrueVertex(commandBuffer, count);

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
			// 渲染线程循环等待Kick信号，收到信号后，设置为Busy信号并开始工作
			renderThread->WaitAndSet(RenderThread::State::Kick, RenderThread::State::Busy);
		}
		// 工作就是把缓存命令全部执行
		s_CommandQueue[GetRenderQueueIndex()]->Execute();  // ???：感觉有问题，这怎么总在执行下一帧的命令（解决：他在执行NextFrame（）时，切换到了下一帧，这里再切换一下又回去了，所以这套逻辑只在一共2个缓冲区的时候没问题）再次解决：NextFrame已经切换了缓冲区，这里要执行的是上一个缓冲区，所以要切换，不过还是再能有2个缓冲区这代码才能跑

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
