#include "hzpch.h"
#include "GridPass.h"
#include <Hazel/Core/Application.h>
#include "Hazel/Utils/FileSystem.h"
namespace GameEngine {
	void GridPass::Init()
	{
		auto RHI = APP_DYNAMCIRHI;
		std::vector<uint8_t> m_Data;
		FileSystem::LoadBinary("Assets/Shader/spv/gridVert.spv", m_Data);
		RHIShaderInfo info;
		info.code = m_Data;
		info.frequency = SHADER_FREQUENCY_VERTEX;
		m_VertShader = RHI->CreateShader(info);
		m_Data.clear();
        FileSystem::LoadBinary("Assets/Shader/spv/gridFrag.spv", m_Data);
        info.code = m_Data;
        info.frequency = SHADER_FREQUENCY_FRAGMENT;
        m_FragShader = RHI->CreateShader(info);

		RHIRootSignatureInfo rootSignatureInfo = {};
		rootSignatureInfo.AddEntryFromReflect(m_VertShader).AddEntryFromReflect(m_FragShader);
		m_RootSignature = RHI->CreateRootSignature(rootSignatureInfo);
		m_RootSignature->CreateDescriptorSet(0);
		RHIGraphicsPipelineInfo pipelineInfo = {};
        pipelineInfo.rootSignature = m_RootSignature;
        pipelineInfo.vertexShader = m_VertShader;
        pipelineInfo.fragmentShader = m_FragShader;
		pipelineInfo.colorAttachmentFormats[0] = FORMAT_R8G8B8A8_UNORM;

		m_Pipeline = RHI->CreateGraphicsPipeline(pipelineInfo);
		LOG_INFO("GridPass::Init()");
	}

	void GridPass::Build(RDGBuilder& builder)
	{
		auto [w, h] = APP_WINDOWSIZE;
		// 创建贴图结点、设置各种格式、Finish返回这个结点的ID
		RDGTextureHandle outColor = builder.CreateTexture("Bloom Out Color")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R8G8B8A8_UNORM)
			.ArrayLayers(1)
			.MipLevels(1)
			.MemoryUsage(MEMORY_USAGE_GPU_ONLY)
			.AllowReadWrite()
			.AllowRenderTarget()
			.Finish();


		// 创建Pass结点
		RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
			.RootSignature(m_RootSignature)
			.Color(0, outColor)
			.Execute([&](RDGPassContext context) {
			auto [w, h] = APP_WINDOWSIZE;

			RHICommandListRef command = context.command;

			command->SetGraphicsPipeline(m_Pipeline);
			command->SetViewport({ 0, 0 }, { w, h });
			command->SetScissor({ 0, 0 }, { w, h });
			command->SetDepthBias(0.0f, 0.0f, 0.0f);
			command->BindDescriptorSet(context.descriptors[0], 0);
			command->Draw(3,1,0,0);

        }).OutputReadWrite(outColor)
        .Finish();
	}

}