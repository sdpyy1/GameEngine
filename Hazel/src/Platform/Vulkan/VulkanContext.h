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
		virtual void Init() override;
		virtual void SwapBuffers() override;
		static Ref<VulkanContext> Get() { return std::dynamic_pointer_cast<VulkanContext>(Application::Get().GetRenderContext()); }
		static VkInstance GetInstance() { return Get()->GetInstanceNative(); }
		static Ref<VulkanDevice> GetDevice() { return Get()->GetDeviceNative(); }
		VkInstance GetInstanceNative() { return m_Instance; }
		Ref<VulkanDevice> GetDeviceNative() { return m_Device; }
	private:
		void createInstance();
		void createPipelineCache();
		void CreateDevice();
		void CheckVersion();

	private:
		Ref<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref<VulkanDevice> m_Device;
		VkInstance m_Instance = nullptr;
		GLFWwindow* window = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPipelineCache m_PipelineCache = nullptr;
	};
}


