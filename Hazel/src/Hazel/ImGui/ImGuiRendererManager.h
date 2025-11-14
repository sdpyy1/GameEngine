#pragma once

#include "Hazel/Core/Layer.h"

#include "Hazel/Events/ApplicationEvent.h"
#include "Hazel/Events/KeyEvent.h"
#include "Hazel/Events/MouseEvent.h"
#include <Hazel/Renderer/Image.h>

namespace Hazel {

	class ImGuiRendererManager
	{
	public:
		virtual void Begin() = 0;
		virtual void End() = 0;
		virtual void Init() = 0;
		void SetDarkThemeV2Colors();

		static std::shared_ptr<ImGuiRendererManager> Create();
	};

}
