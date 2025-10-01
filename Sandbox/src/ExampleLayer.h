#pragma once

#include "Hazel.h"

class ExampleLayer : public Hazel::Layer
{
public:
	ExampleLayer();
	virtual ~ExampleLayer() = default;

	virtual void OnAttach() override;
	virtual void OnDetach() override;

	void OnUpdate(Hazel::Timestep ts) override;
	virtual void OnImGuiRender() override;
	void OnEvent(Hazel::Event& e) override;
private:
	Hazel::ShaderLibrary m_ShaderLibrary;
	Hazel::Ref_old<Hazel::Shader> m_Shader;
	Hazel::Ref_old<Hazel::VertexArray> m_VertexArray;

	Hazel::Ref_old<Hazel::Shader> m_FlatColorShader;
	Hazel::Ref_old<Hazel::VertexArray> m_SquareVA;

	Hazel::Ref_old<Hazel::Texture2D> m_Texture, m_ChernoLogoTexture;

	Hazel::OrthographicCameraController m_CameraController;
	glm::vec3 m_SquareColor = { 0.2f, 0.3f, 0.8f };
};

