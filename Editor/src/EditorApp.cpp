#include <Engine.h>
#include <Hazel/Core/EntryPoint.h>

#include "EditorLayer.h"

namespace Hazel {

	class EngineEditor : public Application
	{
	public:
		EngineEditor()
			: Application("Engine Editor")
		{
			PushLayer(new EditorLayer());
		}

		~EngineEditor()
		{
		}
	};

	Application* CreateApplication()
	{
		return new EngineEditor();
	}

}