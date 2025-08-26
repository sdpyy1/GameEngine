#pragma once
extern Engine::Application* Engine::CreateApplication();
#ifdef ENGINE_PLATFORM_WINDOWS
int main(int argc, char** argv) {
	auto app = Engine::CreateApplication();
	app->Run();
	delete app;
}
#endif