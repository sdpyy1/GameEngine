#include "pch.h"
#include "WindowsInput.h"


#include "Engine/Core/Application.h"
#include "WindowsWindow.h"
#include <GLFW/glfw3.h>

namespace Engine {

	Input* Input::s_Instance = new WindowsInput();

	bool WindowsInput::IsKeyPressedImpl(int keycode)
	{
		WindowsWindow& windowsWindow = (WindowsWindow&)Application::Get().GetWindow();
		auto window = static_cast<GLFWwindow*>(windowsWindow.GetNativeWindow());
		auto state = glfwGetKey(window, keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool WindowsInput::IsMouseButtonPressedImpl(int button)
	{
		WindowsWindow& windowsWindow = (WindowsWindow&)Application::Get().GetWindow();
		auto window = static_cast<GLFWwindow*>(windowsWindow.GetNativeWindow());
		auto state = glfwGetMouseButton(window, button);
		return state == GLFW_PRESS;
	}

	std::pair<float, float> WindowsInput::GetMousePositionImpl()
	{
		WindowsWindow& windowsWindow = (WindowsWindow&)Application::Get().GetWindow();
		auto window = static_cast<GLFWwindow*>(windowsWindow.GetNativeWindow());
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		return { (float)xpos, (float)ypos };
	}

	float WindowsInput::GetMouseXImpl()
	{
		auto [x, y] = GetMousePositionImpl();
		return x;
	}

	float WindowsInput::GetMouseYImpl()
	{
		auto [x, y] = GetMousePositionImpl();
		return y;
	}

}