#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Window/WindowManager.h"
#include "Hazel/Renderer/RHI/Vulkan/VulkanRHI.h"
int main(int argc, char** argv)
{
	GameEngine::Log::Init();
    GameEngine::WindowSpec spec;
    spec.Title = "Hazel";
    spec.Width = 800;
    spec.Height = 600;
    GameEngine::WindowManager * window = new GameEngine::WindowManager(spec);
	//test
	GameEngine::RHIConfig config;
    config.debug = true;
    config.enableRayTracing = false;
	config.api = GameEngine::API::Vulkan;
	GameEngine::DynamicRHI::Init(config);
    GameEngine::RHIQueueInfo aaaa;
    aaaa.index = 0;
    aaaa.type = GameEngine::QueueType::QUEUE_TYPE_GRAPHICS;
    GameEngine::RHISwapchainInfo a;
    a.extent = { 800, 600 };
    a.format = GameEngine::RHIFormat::FORMAT_R8G8B8A8_UNORM;
    a.imageCount = 3;
    a.presentQueue = GameEngine::DynamicRHI::Get()->GetQueue(aaaa);
    a.surface = GameEngine::DynamicRHI::Get()->CreateSurface(window->GetGLFWWindow());
    GameEngine::RHISwapchainRef swpeChain = GameEngine::DynamicRHI::Get()->CreateSwapChain(a);
    while (!glfwWindowShouldClose(window->GetGLFWWindow())) {
        swpeChain->GetNewFrame(nullptr,nullptr);
        swpeChain->Present(nullptr);
        glfwPollEvents();
    }
    
	/*GameEngine::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	GameEngine::Application* app = new GameEngine::Application(spec);
	app->Tick();
	delete app;*/
}
