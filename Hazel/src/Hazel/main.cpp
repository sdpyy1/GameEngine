#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RHI/Vulkan/VulkanRHI.h"
int main(int argc, char** argv)
{
	Hazel::Log::Init();


	// test


	Hazel::RHIConfig config;
    config.debug = true;
    config.enableRayTracing = false;
	config.api = Hazel::API::Vulkan;
	Hazel::DynamicRHI::Init(config);







	/*Hazel::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	Hazel::Application* app = new Hazel::Application(spec);
	app->Tick();
	delete app;*/
}
