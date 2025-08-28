#include <Engine.h>
class Sandbox : public Engine::Application {
public:
	Sandbox() {
	}
	~Sandbox() {
	}
};

// 在外部应用中给出CreateApplication的实现，具体调用在Engine/EntryPoint.h中
Engine::Application* Engine::CreateApplication() {
	return new Sandbox();
}