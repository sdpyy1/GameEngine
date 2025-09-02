#pragma once

// extern: 告诉编译器这个函数在别的地方定义
extern Engine::Application* Engine::CreateApplication();

#ifdef ENGINE_PLATFORM_WINDOWS
int main(int argc, char** argv)
{
	Engine::Log::Init();

	ENGINE_PROFILE_BEGIN_SESSION("Startup", "HazelProfile-Startup.json");
	auto app = Engine::CreateApplication();
	ENGINE_PROFILE_END_SESSION();

	ENGINE_PROFILE_BEGIN_SESSION("Runtime", "HazelProfile-Runtime.json");
	app->Run();
	ENGINE_PROFILE_END_SESSION();

	ENGINE_PROFILE_BEGIN_SESSION("Startup", "HazelProfile-Shutdown.json");
	delete app;
	ENGINE_PROFILE_END_SESSION();
}
#endif