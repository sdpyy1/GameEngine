#pragma once
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderContext.h"
#include <Hazel/Core/Application.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"
namespace Hazel {
	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(void* windowHandle);
		virtual void Init() override;

		// static func
		static Ref<VulkanContext> Get() { return Application::Get().GetRenderContext().As<VulkanContext>(); }
		static VkInstance GetInstance() { return Get()->GetInstanceNative(); }
		static Ref<VulkanDevice> GetCurrentDevice() { return Get()->GetDeviceNative(); }

		// Get func
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
		void* window = nullptr; // 改成GLFW*传参数会报错
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPipelineCache m_PipelineCache = nullptr;
	};
}


