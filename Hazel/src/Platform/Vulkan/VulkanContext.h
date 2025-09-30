#pragma once
#include "Hazel/Renderer/GraphicsContext.h"
#include <Hazel/Core/Application.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"

struct GLFWwindow;

namespace Hazel {

	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		void CheckVersion();
		void CreateDevice();
		virtual void Init() override;
		virtual void SwapBuffers() override;
		void createPipelineCache();
		static VulkanContext Get() { return *Application::Get().GetRenderContext(); }
		static VkInstance GetInstance() { return Get().instance; }

	protected:
		void createInstance();

	private:
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice> m_Device;
		VkInstance instance;
		GLFWwindow* window = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPipelineCache m_PipelineCache = nullptr;
	};
}


