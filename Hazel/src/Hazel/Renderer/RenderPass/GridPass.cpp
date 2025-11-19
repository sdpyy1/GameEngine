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
		builder.AddPass(GetName());
	}

}