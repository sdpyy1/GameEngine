#include <Engine.h>


#include <Engine/Core/EntryPoint.h>

#include "Sandbox2D.h"

class Sandbox : public Engine::Application
{
public:
	Sandbox()
	{
		// PushLayer(new ExampleLayer());
		PushLayer(new Sandbox2D());
	}

	~Sandbox()
	{
	}
};

Engine::Application* Engine::CreateApplication()
{
	return new Sandbox();
}