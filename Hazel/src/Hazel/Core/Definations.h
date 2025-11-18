#pragma once
namespace GameEngine {
#define FRAMES_IN_FLIGHT 3							//Ö¡»º³åÊýÄ¿

#define APP_FRAMEINDEX Application::GetFrameIndex()
#define APP_GLFWWINDOW Application::GetWindowManager()->GetGLFWWindow()
#define APP_WINDOWMINIMIZED Application::Get().isMinimized()
#define SWAPCHAIN_COLOR_FORMAT FORMAT_R8G8B8A8_UNORM
}