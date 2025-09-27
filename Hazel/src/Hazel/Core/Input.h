#pragma once

#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"

#include <glm/glm.hpp>

namespace Hazel {

	class Input
	{
	public:
		static bool IsKeyPressed(KeyCode key);
		// 新增：鼠标模式枚举
		enum class CursorMode
		{
			Normal,   // 正常（显示+自由移动）
			Hidden,   // 隐藏（不显示，但可移出窗口）
			Locked    // 锁定（隐藏+锁定在窗口中心，循环移动）
		};

		// 新增：设置鼠标模式
		static void Input::SetCursorMode(CursorMode mode);

		static bool IsMouseButtonPressed(MouseCode button);
		static glm::vec2 GetMousePosition();
		static float GetMouseX();
		static float GetMouseY();
	};
}
