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
		RDGTextureHandle outColor = builder.CreateTexture("RDG_TEXTURE_GRID")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_R8G8B8A8_UNORM)
			.AllowRenderTarget()
			.Finish();
		RDGTextureHandle outDepth = builder.CreateTexture("RDG_TEXTURE_GRID_DEPTH")
			.Exetent({ w, h, 1 })
			.Format(FORMAT_D32_SFLOAT)
			.AllowDepthStencil()
			.Finish();

		// 创建Pass结点
		RDGRenderPassHandle pass = builder.CreateRenderPass(GetName())
			.RootSignature(m_RootSignature)
			.Color(0, outColor, ATTACHMENT_LOAD_OP_CLEAR, ATTACHMENT_STORE_OP_STORE)
			.Execute([&](RDGPassContext context) {
			auto [w, h] = APP_WINDOWSIZE;

			RHICommandListRef command = context.command;

			command->SetGraphicsPipeline(m_Pipeline);
			command->SetViewport({ 0, 0 }, { w, h });
			command->SetScissor({ 0, 0 }, { w, h });
			command->SetDepthBias(0.0f, 0.0f, 0.0f);
			command->BindDescriptorSet(context.descriptors[0], 0);
			command->Draw(6,1,0,0);

        })
        .Finish();
	}

}