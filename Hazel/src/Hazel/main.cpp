#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Window/WindowManager.h"
#include "Hazel/Renderer/RHI/Vulkan/VulkanRHI.h"
int main(int argc, char** argv)
{
	GameEngine::Log::Init();
    
    
	GameEngine::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	GameEngine::Application* app = new GameEngine::Application(spec);
	app->Tick();
	delete app;
	
}
