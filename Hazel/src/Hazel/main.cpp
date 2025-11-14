#include "hzpch.h"
#include "Hazel/Core/Application.h"

int main(int argc, char** argv)
{
	Hazel::Log::Init();

	Hazel::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	Hazel::Application* app = new Hazel::Application(spec);
	app->Run();
	delete app;
}
