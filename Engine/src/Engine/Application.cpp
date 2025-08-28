#include "Application.h"
#include "Log.h"
#include "Events/ApplicationEvent.h"


namespace Engine {
	Application::Application()
	{
	}

	Application::~Application()
	{
	}
	void Application::Run()
	{
		WindowResizeEvent e(1280, 720);
		if (e.IsInCategory(EventCategoryApplication))
		{
			ENGINE_ERROR(e.ToString());
		}
		if (e.IsInCategory(EventCategoryInput))
		{
			ENGINE_TRACE(e.ToString());
		}
		while (true) {

		}
	}
}

