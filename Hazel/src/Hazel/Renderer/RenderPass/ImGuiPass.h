#pragma once
#include "RenderPass.h"
namespace GameEngine {
	class ImGuiPass :public RenderPassNew
	{
	public:
		ImGuiPass() = default;
		~ImGuiPass() = default;

		virtual void Init() override final;

		virtual void Build(RDGBuilder& builder) override final;

		virtual std::string GetName() { return "ImGuiPass"; }
		virtual PassType GetType() override final { return IMGUI_PASS; }
	private:
			RHIShaderRef m_VertShader;
			RHIShaderRef m_FragShader;
			RHIRootSignatureRef m_RootSignature;
			RHIGraphicsPipelineRef m_Pipeline;
	};

}