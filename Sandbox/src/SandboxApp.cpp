#include <Engine.h>
#include <imgui.h>
class Examplelayer : public Engine::Layer {
public:
	Examplelayer() :Layer("Example") {
	}
	void OnUpdate() override {
		if(Engine::Input::IsKeyPressed(ENGINE_KEY_TAB))
			ENGINE_ERROR("Tab key is pressed!");
	}

	void OnEvent(Engine::Event& event) override
	{
		if (event.GetEventType() == Engine::EventType::KeyPressed)
		{
			Engine::KeyPressedEvent& e = (Engine::KeyPressedEvent&)event;
			if (e.GetKeyCode() == ENGINE_KEY_TAB)
				ENGINE_TRACE("Tab key is pressed (event)!");
			ENGINE_TRACE("{0}", (char)e.GetKeyCode());
		}
	}
	void OnImGuiRender() override {
		ImGui::Begin("Hello");
		ImGui::Text("This is some useful text.");
		ImGui::End();
	}

};
class Sandbox : public Engine::Application {
public:
	Sandbox() {
		PushLayer(new Examplelayer());
	}
	~Sandbox() {
	}
};

// 在外部应用中给出CreateApplication的实现，具体调用在Engine/EntryPoint.h中
Engine::Application* Engine::CreateApplication() {
	return new Sandbox();
}