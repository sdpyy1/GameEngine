#include "hzpch.h"
#include "EnvPass.h"
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Asset/Model/Material.h"
namespace Hazel{
	void EnvPass::Init()
	{
		// HDR->Cubemap Pass
		Ref<Shader> equirectangularConversionShader = Renderer::GetShaderLibrary()->Get("EquirectangularToCubeMap");
		ComputePassSpecification equirectangularSpec;
		equirectangularSpec.DebugName = "EquirectangularToCubeMap";
		equirectangularSpec.Pipeline = PipelineCompute::Create(equirectangularConversionShader);
		equirectangularSpec.moreDescriptors = 1;
		m_EquirectangularPass = ComputePass::Create(equirectangularSpec);

		// irradianceMap Pass
		Ref<Shader> environmentIrradianceShader = Renderer::GetShaderLibrary()->Get("EnvironmentIrradiance");
		ComputePassSpecification environmentIrradianceSpec;
		environmentIrradianceSpec.DebugName = "EnvironmentIrradiance";
		environmentIrradianceSpec.Pipeline = PipelineCompute::Create(environmentIrradianceShader);
		m_EnvironmentIrradiancePass = ComputePass::Create(environmentIrradianceSpec);

		// PreFilterCubeMap Pass
		Ref<Shader> preFilterShader = Renderer::GetShaderLibrary()->Get("EnvironmentMipFilter");
		ComputePassSpecification preFilterSpec;
		preFilterSpec.DebugName = "EnvironmentMipFilter";
		preFilterSpec.Pipeline = PipelineCompute::Create(preFilterShader);
		preFilterSpec.moreDescriptors = 12;
		m_EnvironmentPreFilterPass = ComputePass::Create(preFilterSpec);

	}

	EnvTextures EnvPass::compute(std::filesystem::path path,Ref<RenderCommandBuffer> m_CommandBuffer)
	{
		if (Envs.find(path) != Envs.end())
		{
			return Envs[path];
		}
		else
		{
			EnvTextures env;
			const uint32_t cubemapSize = Renderer::GetConfig().EnvironmentMapResolution;
			const uint32_t irradianceMapSize = Renderer::GetConfig().IrradianceMapSize;
			TextureSpecification cubemapSpec;
			cubemapSpec.Format = ImageFormat::RGBA32F;
			cubemapSpec.Width = cubemapSize;
			cubemapSpec.Height = cubemapSize;
			cubemapSpec.Storage = true;
			cubemapSpec.GenerateMips = false;
			env.m_EnvCubeMap = TextureCube::Create(cubemapSpec);
			env.m_EnvPreFilterMap = TextureCube::Create(cubemapSpec);

			TextureSpecification IrradianceMapSpec;
			IrradianceMapSpec.Format = ImageFormat::RGBA32F;
			IrradianceMapSpec.Width = irradianceMapSize;
			IrradianceMapSpec.Height = irradianceMapSize;
			IrradianceMapSpec.Storage = true;
			IrradianceMapSpec.GenerateMips = false;
			env.m_EnvIrradianceMap = TextureCube::Create(IrradianceMapSpec);
			env.m_EnvLut = Renderer::GetBRDFLutTexture();
			if (path == "") {
                env.m_EnvEquirect = Renderer::GetBlackTexture();
			}
			else {
				env.m_EnvEquirect = Texture2D::Create(TextureSpecification(), path);
				ASSERT(env.m_EnvEquirect->GetFormat() == ImageFormat::RGBA32F, "Texture is not HDR!");
			}
			//if (isInit) {
			//	m_CommandBuffer->Begin();
			//}

			// HDR转CubemapPass
			Renderer::BeginComputePass(m_CommandBuffer, m_EquirectangularPass);
			m_EquirectangularPass->SetInput("u_EquirectangularTex",env.m_EnvEquirect);
			m_EquirectangularPass->SetInput("o_CubeMap",env.m_EnvCubeMap);
			Renderer::DispatchCompute(m_CommandBuffer, m_EquirectangularPass, nullptr, glm::ivec3(cubemapSize / 32, cubemapSize / 32, 6));
			Renderer::EndComputePass(m_CommandBuffer, m_EquirectangularPass);

			Renderer::Submit([=]() mutable{
				env.m_EnvCubeMap->GenerateMips(m_CommandBuffer); // DispatchCompute无法自动生成mipmap，所以这里手动生成，在MipFilter计算中会使用
				});
			// PreFilterCubeMap第一层用HDR
			Renderer::BeginComputePass(m_CommandBuffer, m_EquirectangularPass);
			m_EquirectangularPass->SetInput(env.m_EnvEquirect, 1, 0);
			m_EquirectangularPass->SetInput(env.m_EnvPreFilterMap, 0, InputType::stoage, 0);
			Renderer::DispatchCompute(m_CommandBuffer, m_EquirectangularPass, nullptr, glm::ivec3(cubemapSize / 32, cubemapSize / 32, 6), 0);
			Renderer::EndComputePass(m_CommandBuffer, m_EquirectangularPass);

			// 环境Irradiance
			Renderer::BeginComputePass(m_CommandBuffer, m_EnvironmentIrradiancePass);
			m_EnvironmentIrradiancePass->SetInput("u_RadianceMap",env.m_EnvCubeMap);
			m_EnvironmentIrradiancePass->SetInput("o_IrradianceMap",env.m_EnvIrradianceMap);
			Renderer::DispatchCompute(m_CommandBuffer, m_EnvironmentIrradiancePass, nullptr, glm::ivec3(irradianceMapSize / 32, irradianceMapSize / 32, 6), Buffer(&Renderer::GetConfig().IrradianceMapComputeSamples, sizeof(uint32_t)));
			Renderer::Submit([=]() mutable{
				env.m_EnvIrradianceMap->GenerateMips(m_CommandBuffer);
				});
			Renderer::EndComputePass(m_CommandBuffer, m_EnvironmentIrradiancePass);

			// 环境MipFilter
			Renderer::BeginComputePass(m_CommandBuffer, m_EnvironmentPreFilterPass);
			uint32_t mipCount = env.m_EnvCubeMap->GetMipLevelCount();
			for (uint32_t i = 1; i < mipCount; i++) {
				m_EnvironmentPreFilterPass->SetInput(env.m_EnvPreFilterMap, 0, InputType::stoage, i, i);
				m_EnvironmentPreFilterPass->SetInput(env.m_EnvCubeMap, 1, InputType::sampler, i);
			}
			const float deltaRoughness = 1.0f / glm::max((float)mipCount - 1.0f, 1.0f);
			for (uint32_t i = 1, size = cubemapSize; i < mipCount; i++, size /= 2)
			{
				uint32_t numGroups = glm::max(1u, size / 32);
				float roughness = i * deltaRoughness;
				Renderer::DispatchCompute(m_CommandBuffer, m_EnvironmentPreFilterPass, nullptr, glm::ivec3(numGroups, numGroups, 6), i, { &roughness, sizeof(float) });
			}
			Renderer::EndComputePass(m_CommandBuffer, m_EnvironmentPreFilterPass);
	/*		if (isInit) {
				m_CommandBuffer->End();
				m_CommandBuffer->Submit();
			}*/


            Envs[path] = env;
			LOG_INFO("Compute Env Success");
            return Envs[path];
		}
		
	}

}

