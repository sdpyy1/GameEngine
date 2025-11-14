#pragma once

#include "Hazel/Core/KeyCodes.h"
#include <map>
#include <glm/glm.hpp>

namespace Hazel {
	class Input
	{
	public:
		static bool IsKeyDown(KeyCode keycode);
		static bool IsMouseButtonDown(MouseButton button);
		static float GetMouseX();
		static float GetMouseY();
		static std::pair<float, float> GetMousePosition();

		static void SetCursorMode(CursorMode mode);
		static CursorMode GetCursorMode();
	};
}
