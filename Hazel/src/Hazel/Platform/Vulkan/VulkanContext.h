#pragma once
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderContext.h"
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
namespace Hazel {
	class Application;
	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(void* windowHandle);
		virtual void Init() override;

		static Ref<VulkanContext> Get();
		static VkInstance GetInstance();
		static Ref<VulkanDevice> GetCurrentDevice();
		VkPipelineCache GetPipelineCache() { return m_PipelineCache; }
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
