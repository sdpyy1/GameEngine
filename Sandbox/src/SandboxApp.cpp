#include <Engine.h>
#include "glm/glm.hpp"

class Examplelayer : public Engine::Layer {
public:
	Examplelayer() :Layer("Example") {
		ENGINE_INFO("ExampleLayer Constructor");
	}
	void OnUpdate() override {
		ENGINE_INFO("ExampleLayer::Update");
		if(Engine::Input::IsKeyPressed(ENGINE_KEY_TAB))
			ENGINE_ERROR("Tab key is pressed!");
	}

	void OnEvent(Engine::Event& event) override
	{
		ENGINE_TRACE("{0}", event.ToString());
	}

};
class Sandbox : public Engine::Application {
public:
	Sandbox() {
		PushLayer(new Examplelayer());
		PushOverlay(new Engine::ImGuiLayer());
	}
	~Sandbox() {
	}
};

// 在外部应用中给出CreateApplication的实现，具体调用在Engine/EntryPoint.h中
Engine::Application* Engine::CreateApplication() {
	return new Sandbox();
}