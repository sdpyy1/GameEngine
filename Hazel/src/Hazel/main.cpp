#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Renderer/RHI/Vulkan/VulkanRHI.h"
int main(int argc, char** argv)
{
	Hazel::Log::Init();

    // 1. 初始化 GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW!" << std::endl;
        return -1;
    }

    // 2. 告诉 GLFW 不创建 OpenGL 上下文
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    // 3. 可选：隐藏窗口
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    // 4. 创建窗口
    GLFWwindow* window = glfwCreateWindow(800, 600, "Vulkan Test", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window!" << std::endl;
        glfwTerminate();
        return -1;
    }
	//test
	Hazel::RHIConfig config;
    config.debug = true;
    config.enableRayTracing = false;
	config.api = Hazel::API::Vulkan;
	Hazel::DynamicRHI::Init(config);
    Hazel::RHIQueueInfo aaaa;
    aaaa.index = 0;
    aaaa.type = Hazel::QueueType::QUEUE_TYPE_GRAPHICS;
    Hazel::RHISwapchainInfo a;
    a.extent = { 800, 600 };
    a.format = Hazel::RHIFormat::FORMAT_R8G8B8A8_UNORM;
    a.imageCount = 3;
    a.presentQueue = Hazel::DynamicRHI::Get()->GetQueue(aaaa);
    a.surface = Hazel::DynamicRHI::Get()->CreateSurface(window);
	Hazel::DynamicRHI::Get()->CreateSwapChain(a);
    
	/*Hazel::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	Hazel::Application* app = new Hazel::Application(spec);
	app->Tick();
	delete app;*/
}
