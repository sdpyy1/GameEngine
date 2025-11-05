#include "hzpch.h"
#include "Hazel/Renderer/Renderer.h"
#include "Platform/Vulkan/VulkanRenderer.h"
#include "Hazel/Renderer/RendererAPI.h"
#include <glm/gtc/random.hpp>
#include <imgui.h>
#include <Platform/Vulkan/VulkanImage.h>

#include "Hazel/Asset/Model/Material.h"
namespace Hazel {
	// 这里存储常用渲染资源
	struct RendererData
	{
		Ref<ShaderLibrary> m_ShaderLibrary;
		Ref<TextureCube> BlackCubeTexture;
		Ref<Texture2D> BRDFLutTexture;
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2D> BlackTexture;
	};

	static RendererConfig s_Config;
	static RendererData* s_Data = nullptr;
	constexpr static uint32_t s_RenderCommandQueueCount = 2; // 目前代码只支持=2
	static RenderCommandQueue* s_CommandQueue[s_RenderCommandQueueCount];
	static std::atomic<uint32_t> s_RenderCommandQueueSubmissionIndex = 0;
	static RenderCommandQueue s_ResourceFreeQueue[3];
	static RendererAPI* s_RendererAPI = nullptr;

	RendererConfig& Renderer::GetConfig()
	{
		return s_Config;
	}

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
		Shader::DescriptorBinding cameraDataBinding;
		cameraDataBinding.binding = 0;
		cameraDataBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		cameraDataBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		cameraDataBinding.count = 1;
		cameraDataBinding.set = 0;
		Shader::DescriptorBinding boneTrasnfromBinding;
		boneTrasnfromBinding.binding = 1;
		boneTrasnfromBinding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		boneTrasnfromBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		boneTrasnfromBinding.count = 1;
		boneTrasnfromBinding.set = 0;
		Shader::PushConstantRange BoneInfluencePushRange;
		BoneInfluencePushRange.shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
		BoneInfluencePushRange.offset = 0;
		BoneInfluencePushRange.size = 4;
		// GBufferPass
		{
			Shader::ShaderSpecification gbufferShaderSpec;
			gbufferShaderSpec.bindings.push_back(cameraDataBinding);
			Shader::DescriptorBinding shadowBinding;
            shadowBinding.binding = 2;
            shadowBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            shadowBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            shadowBinding.count = 1;
            shadowBinding.set = 0;
            gbufferShaderSpec.bindings.push_back(shadowBinding);
			Shader::DescriptorBinding RendererDataBinding;
            RendererDataBinding.binding = 3;
            RendererDataBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            RendererDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            RendererDataBinding.count = 1;
            RendererDataBinding.set = 0;
            gbufferShaderSpec.bindings.push_back(RendererDataBinding);
			Shader::DescriptorBinding SceneDataBinding;
            SceneDataBinding.binding = 4;
            SceneDataBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            SceneDataBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            SceneDataBinding.count = 1;
            SceneDataBinding.set = 0;
            gbufferShaderSpec.bindings.push_back(SceneDataBinding);

			Shader::DescriptorBinding u_ShadowMapTextureBinding;
            u_ShadowMapTextureBinding.binding = 5;
            u_ShadowMapTextureBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            u_ShadowMapTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            u_ShadowMapTextureBinding.count = 4;
            u_ShadowMapTextureBinding.set = 0;
            gbufferShaderSpec.bindings.push_back(u_ShadowMapTextureBinding);

			Shader::DescriptorBinding textureBinding;
            textureBinding.binding = 6;
            textureBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            textureBinding.count = 1;
            textureBinding.set = 0;
            gbufferShaderSpec.bindings.push_back(textureBinding);
			textureBinding.binding = 7;
			gbufferShaderSpec.bindings.push_back(textureBinding);
			textureBinding.binding = 8;
			gbufferShaderSpec.bindings.push_back(textureBinding);
			
			// 2. Albedo 贴图（combined image sampler）
			Shader::DescriptorBinding albedoBinding;
			albedoBinding.binding = 0;
			albedoBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			albedoBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			albedoBinding.count = 1;
			albedoBinding.set = 1;
			gbufferShaderSpec.bindings.push_back(albedoBinding);
			// 3. Normal 贴图
			Shader::DescriptorBinding normalBinding;
			normalBinding.binding = 1;
			normalBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			normalBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			normalBinding.count = 1;
			normalBinding.set = 1;
			gbufferShaderSpec.bindings.push_back(normalBinding);
			// 4. Metalness 贴图
			Shader::DescriptorBinding metalnessBinding;
			metalnessBinding.binding = 2;
			metalnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			metalnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			metalnessBinding.count = 1;
			metalnessBinding.set = 1;
			gbufferShaderSpec.bindings.push_back(metalnessBinding);
			// 5. Roughness 贴图
			Shader::DescriptorBinding roughnessBinding;
			roughnessBinding.binding = 3;
			roughnessBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			roughnessBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			roughnessBinding.count = 1;
			roughnessBinding.set = 1;
			gbufferShaderSpec.bindings.push_back(roughnessBinding);

			Shader::DescriptorBinding emsBinding;
			emsBinding.binding = 4;
			emsBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			emsBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			emsBinding.count = 1;
			emsBinding.set = 1;
			gbufferShaderSpec.bindings.push_back(emsBinding);
			Shader::PushConstantRange PushRange;
            PushRange.shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT ;
            PushRange.offset = 0;
            PushRange.size = sizeof(MaterialPush);
            gbufferShaderSpec.pushConstantRanges.push_back(PushRange);
			s_Data->m_ShaderLibrary->LoadCommonShader("gBuffer", gbufferShaderSpec);
			Shader::PushConstantRange PushRange1;
			PushRange1.shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
            PushRange1.size = 4;
			PushRange1.offset =sizeof(MaterialPush);
			gbufferShaderSpec.pushConstantRanges.push_back(PushRange1);
			gbufferShaderSpec.bindings.push_back(boneTrasnfromBinding);
			s_Data->m_ShaderLibrary->LoadCommonShader("gBufferAnim", gbufferShaderSpec);
		}
		// DirShadowPass
		{
			Shader::ShaderSpecification shadowShaderSpec;
			shadowShaderSpec.bindings.push_back(cameraDataBinding);
			Shader::PushConstantRange casPushRange;
			casPushRange.shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
			casPushRange.offset = 0;
			casPushRange.size = 4;
			shadowShaderSpec.pushConstantRanges = { casPushRange };
			s_Data->m_ShaderLibrary->LoadCommonShader("DirShadowMap", shadowShaderSpec);
			shadowShaderSpec.bindings.push_back(boneTrasnfromBinding);
			shadowShaderSpec.pushConstantRanges[0].size = 8;
			s_Data->m_ShaderLibrary->LoadCommonShader("DirShadowMapAnim", shadowShaderSpec);
		}
		// SpotShadowPass
		{
			Shader::ShaderSpecification shadowShaderSpec;
			shadowShaderSpec.bindings.push_back(cameraDataBinding); // 并不是摄像机数据是光照矩阵
			Shader::PushConstantRange PushRange;
			PushRange.shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
			PushRange.offset = 0;
			PushRange.offset = 0;
			PushRange.size = 4;
			shadowShaderSpec.pushConstantRanges = { PushRange };
			s_Data->m_ShaderLibrary->LoadCommonShader("SpotShadowMap", shadowShaderSpec);
			shadowShaderSpec.bindings.push_back(boneTrasnfromBinding);
			shadowShaderSpec.pushConstantRanges[0].size = 8;
			s_Data->m_ShaderLibrary->LoadCommonShader("SpotShadowMapAnim", shadowShaderSpec);
		}
		// GridPass
		{
			Shader::ShaderSpecification gridShaderSpec;
			Shader::DescriptorBinding depthBinding;
			Shader::DescriptorBinding griduboBinding;
			griduboBinding.binding = 0;                  // 对应着色器中 binding = 0
			griduboBinding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			griduboBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
			griduboBinding.count = 1;
			griduboBinding.set = 0;
			depthBinding.binding = 1;
			depthBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			depthBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			depthBinding.count = 1;
			depthBinding.set = 0;
			gridShaderSpec.bindings.push_back(griduboBinding);
			gridShaderSpec.bindings.push_back(depthBinding);
			s_Data->m_ShaderLibrary->LoadCommonShader("grid", gridShaderSpec);
		}
		// PreDepthPass
		{
			Shader::ShaderSpecification preDepthShaderSpec;
			preDepthShaderSpec.bindings.push_back(cameraDataBinding);
			s_Data->m_ShaderLibrary->LoadCommonShader("PreDepth", preDepthShaderSpec);
			preDepthShaderSpec.bindings.push_back(boneTrasnfromBinding);
			preDepthShaderSpec.pushConstantRanges.push_back(BoneInfluencePushRange);
			s_Data->m_ShaderLibrary->LoadCommonShader("PreDepthAnim", preDepthShaderSpec);
		}
		// HZBPass
		{
			Shader::ShaderSpecification hzbShaderSpec;
			Shader::DescriptorBinding binding0;
			binding0.binding = 0;
			binding0.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			binding0.count = 1;
			binding0.set = 0;
			hzbShaderSpec.bindings.push_back(binding0);
			Shader::DescriptorBinding binding1;
			binding1.binding = 1;
			binding1.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			binding1.count = 11;
			binding1.set = 0;
			hzbShaderSpec.bindings.push_back(binding1);
			Shader::PushConstantRange PushRange;
			PushRange.shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
			PushRange.offset = 0;
			PushRange.size = 4;
			hzbShaderSpec.pushConstantRanges = { PushRange };
			s_Data->m_ShaderLibrary->LoadCommonShader("HZB", hzbShaderSpec, true);
		}
		// EquirectangularToCubeMap
		{
			Shader::ShaderSpecification EquirectangularToCubeMapShaderSpec;
			Shader::DescriptorBinding binding0;
			binding0.binding = 0;
			binding0.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			binding0.count = 1;
			binding0.set = 0;
			EquirectangularToCubeMapShaderSpec.bindings.push_back(binding0);
			Shader::DescriptorBinding outputCubeBinding;
			outputCubeBinding.binding = 1;
			outputCubeBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			outputCubeBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			outputCubeBinding.count = 1;
			outputCubeBinding.set = 0;
			EquirectangularToCubeMapShaderSpec.bindings.push_back(outputCubeBinding);
			s_Data->m_ShaderLibrary->LoadCommonShader("EquirectangularToCubeMap", EquirectangularToCubeMapShaderSpec, true);
		}
		// IrradianceMap
		{
			Shader::ShaderSpecification IrradianceMapShaderSpec;
			Shader::DescriptorBinding binding0;
			binding0.binding = 0;
			binding0.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			binding0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			binding0.count = 1;
			binding0.set = 0;
			IrradianceMapShaderSpec.bindings.push_back(binding0);
			Shader::DescriptorBinding outputCubeBinding;
			outputCubeBinding.binding = 1;
			outputCubeBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			outputCubeBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
			outputCubeBinding.count = 1;
			outputCubeBinding.set = 0;
			IrradianceMapShaderSpec.bindings.push_back(outputCubeBinding);
			Shader::PushConstantRange PushRange;
			PushRange.shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
			PushRange.offset = 0;
			PushRange.size = 4;
			IrradianceMapShaderSpec.pushConstantRanges = { PushRange };
			s_Data->m_ShaderLibrary->LoadCommonShader("EnvironmentIrradiance", IrradianceMapShaderSpec, true);
		}
		// PrefilterMap
		{
            Shader::ShaderSpecification PrefilterMapShaderSpec;
            Shader::DescriptorBinding binding0;
            binding0.binding = 0;
            binding0.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            binding0.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding0.count = 1;
            binding0.set = 0;
            PrefilterMapShaderSpec.bindings.push_back(binding0);
            Shader::DescriptorBinding binding1;
            binding1.binding = 1;
            binding1.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding1.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding1.count = 1;
            binding1.set = 0;
            PrefilterMapShaderSpec.bindings.push_back(binding1);
            Shader::PushConstantRange PushRange;
            PushRange.shaderStage = VK_SHADER_STAGE_COMPUTE_BIT;
            PushRange.offset = 0;
            PushRange.size = 4;
            PrefilterMapShaderSpec.pushConstantRanges = { PushRange };
            s_Data->m_ShaderLibrary->LoadCommonShader("EnvironmentMipFilter", PrefilterMapShaderSpec, true);
		}
		
		// lighting
		{
            Shader::ShaderSpecification LightingShaderSpec;
            Shader::DescriptorBinding imageBinding;
            imageBinding.binding = 0;
            imageBinding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            imageBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            imageBinding.count = 1;
            imageBinding.set = 0;
            LightingShaderSpec.bindings.push_back(imageBinding);
			imageBinding.binding = 1;
			LightingShaderSpec.bindings.push_back(imageBinding);
			imageBinding.binding = 2;
			LightingShaderSpec.bindings.push_back(imageBinding);
			imageBinding.binding = 3;
			LightingShaderSpec.bindings.push_back(imageBinding);
            imageBinding.binding = 4;
			LightingShaderSpec.bindings.push_back(imageBinding);
			imageBinding.binding = 5;
			LightingShaderSpec.bindings.push_back(imageBinding);
			imageBinding.binding = 6;
			LightingShaderSpec.bindings.push_back(imageBinding);
			s_Data->m_ShaderLibrary->LoadCommonShader("Lighting", LightingShaderSpec);
		}
		
		
		// 加载纹理
		{
			uint32_t whiteTextureData = 0xffffffff;
			TextureSpecification spec;
			spec.Format = ImageFormat::RGBA;
			spec.Width = 1;
			spec.Height = 1;
			s_Data->WhiteTexture = Texture2D::Create(spec, Buffer(&whiteTextureData, sizeof(uint32_t)));

			constexpr uint32_t blackTextureData = 0xff000000;
			s_Data->BlackTexture = Texture2D::Create(spec, Buffer(&blackTextureData, sizeof(uint32_t)));

			constexpr uint32_t blackCubeTextureData[6] = { 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000, 0xff000000 };
			s_Data->BlackCubeTexture = TextureCube::Create(spec, Buffer(&blackTextureData, sizeof(blackCubeTextureData)));
			{
				TextureSpecification spec;
				spec.SamplerWrap = TextureWrap::Clamp;
				spec.needYFlip = false;
				s_Data->BRDFLutTexture = Texture2D::Create(spec, std::filesystem::path("Assets/Texture/BRDF_LUT.png"));
			}
		}
		// 为并发帧创建了描述符池、提前准备了全屏顶点数据存入了GPU
		s_RendererAPI->Init();
	}
	void Renderer::Shutdown()
	{
	}
	Ref<Texture2D> Renderer::GetBlackTexture()
	{
		return s_Data->BlackTexture;
	}
	Ref<Texture2D> Renderer::GetBRDFLutTexture()
	{
		return s_Data->BRDFLutTexture;
	}

	Ref<ShaderLibrary> Renderer::GetShaderLibrary()
	{
		return s_Data->m_ShaderLibrary;
	}

	void Renderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		return s_RendererAPI->BeginRenderPass(renderCommandBuffer, renderPass, explicitClear);
	}

	void Renderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		return s_RendererAPI->EndRenderPass(renderCommandBuffer);
	}

	void Renderer::BeginComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		HZ_CORE_ASSERT(computePass, "ComputePass cannot be null!");

		s_RendererAPI->BeginComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::EndComputePass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass)
	{
		s_RendererAPI->EndComputePass(renderCommandBuffer, computePass);
	}

	void Renderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, Buffer constants)
	{
		s_RendererAPI->DispatchCompute(renderCommandBuffer, computePass, material, workGroups, constants);
	}

	void Renderer::DispatchCompute(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<ComputePass> computePass, Ref<Material> material, const glm::uvec3& workGroups, uint32_t descrptorSetIndex, Buffer constants)
	{
		s_RendererAPI->DispatchCompute(renderCommandBuffer, computePass, material, workGroups, descrptorSetIndex, constants);
	}

	void Renderer::SwapQueues()
	{
		s_RenderCommandQueueSubmissionIndex = (s_RenderCommandQueueSubmissionIndex + 1) % s_RenderCommandQueueCount;
	}

	void Renderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer)
	{
		return s_RendererAPI->BindVertData(commandBuffer, testVertexBuffer);
	}
	void Renderer::RenderStaticMeshWithMaterial(Ref<RenderCommandBuffer> commandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		return s_RendererAPI->RenderStaticMeshWithMaterial(commandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, instanceCount, additionalUniforms);
	}
	void Renderer::RenderSkeletonMeshWithMaterial(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<Pipeline> pipeline, Ref<MeshSource> meshSource, uint32_t submeshIndex, Ref<Material> material, Ref<VertexBuffer> transformBuffer, uint32_t transformOffset, uint32_t boneTransformsOffset, uint32_t instanceCount, Buffer additionalUniforms)
	{
		s_RendererAPI->RenderSkeletonMeshWithMaterial(renderCommandBuffer, pipeline, meshSource, submeshIndex, material, transformBuffer, transformOffset, boneTransformsOffset, instanceCount, additionalUniforms);
	}
	void Renderer::DrawPrueVertex(Ref<RenderCommandBuffer> commandBuffer, uint32_t count)
	{
		return s_RendererAPI->DrawPrueVertex(commandBuffer, count);
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

	void Renderer::BeginFrame()
	{
		s_RendererAPI->BeginFrame();
	}
	void Renderer::EndFrame()
	{
		s_RendererAPI->EndFrame();
	}

	RenderCommandQueue& Renderer::GetRenderCommandQueue()
	{
		return *s_CommandQueue[s_RenderCommandQueueSubmissionIndex];
	}
	RenderCommandQueue& Renderer::GetRenderResourceReleaseQueue(uint32_t index)
	{
		return s_ResourceFreeQueue[index];
	}
	RendererCapabilities& Renderer::GetCapabilities()
	{
		return s_RendererAPI->GetCapabilities();
	}

	uint32_t Renderer::GetCurrentFrameIndex()
	{
		return Application::Get().GetCurrentFrameIndex();
	}

	uint32_t Renderer::RT_GetCurrentFrameIndex()
	{
		// Swapchain owns the Render Thread frame index
		return Application::Get().GetWindow()->GetSwapChain().GetCurrentBufferIndex();
	}

	Ref<Texture2D> Renderer::GetWhiteTexture()
	{
		return s_Data->WhiteTexture;
	}

	Ref<TextureCube> Renderer::GetBlackCubeTexture()
	{
		return s_Data->BlackCubeTexture;
	}
}
