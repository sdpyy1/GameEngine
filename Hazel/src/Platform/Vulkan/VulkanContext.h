#pragma once
#include "Hazel/Renderer/Renderer.h"
#include "Hazel/Renderer/RenderContext.h"
#include <Hazel/Core/Application.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"
struct GLFWwindow;

namespace Hazel {
	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init() override;

		// static func
		static Ref_old<VulkanContext> Get() { return std::dynamic_pointer_cast<VulkanContext>(Application::Get().GetRenderContext()); }
		static VkInstance GetInstance() { return Get()->GetInstanceNative(); }
		static Ref_old<VulkanDevice> GetCurrentDevice() { return Get()->GetDeviceNative(); }

		// Get func
		VkInstance GetInstanceNative() { return m_Instance; }
		Ref_old<VulkanDevice> GetDeviceNative() { return m_Device; }
	private:
		void createInstance();
		void createPipelineCache();
		void CreateDevice();
		void CheckVersion();

	private:
		Ref_old<VulkanPhysicalDevice> m_PhysicalDevice;
		Ref_old<VulkanDevice> m_Device;
		VkInstance m_Instance = nullptr;
		GLFWwindow* window = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkPipelineCache m_PipelineCache = nullptr;
	};
}


