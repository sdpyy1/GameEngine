#pragma once
#include "core.h"

namespace Engine {
	class ENGINE_API Application
	{
	public:
		Application();
		virtual ~Application();
		void Run();
	};

	// to be defined in CLIENT
	Application* CreateApplication();
}


