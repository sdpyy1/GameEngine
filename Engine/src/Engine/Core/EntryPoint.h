#pragma once

// extern: ���߱�������������ڱ�ĵط�����
extern Engine::Application* Engine::CreateApplication();

#ifdef ENGINE_PLATFORM_WINDOWS
int main(int argc, char** argv) {
	Engine::Log::Init();
	ENGINE_CORE_TRACE("Initialized Log!");
	auto app = Engine::CreateApplication();
	app->Run();
	delete app;
}
#endif