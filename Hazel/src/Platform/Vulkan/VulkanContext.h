#pragma once
#include "Hazel/Renderer/GraphicsContext.h"
#include <Hazel/Core/Application.h>
#include "VulkanDevice.h"
#include "VulkanSwapChain.h"
#include "VulkanContext.h"
struct GLFWwindow;

namespace Hazel {
#if defined(HZ_DEBUG) || defined(HZ_RELEASE)
	static bool s_Validation = true;
#else
	static bool s_Validation = false; // Let's leave this on for now...
#endif
	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init() override;
		virtual void SwapBuffers() override;
		static Ref_old<VulkanContext> Get() { return std::dynamic_pointer_cast<VulkanContext>(Application::Get().GetRenderContext()); }
		static VkInstance GetInstance() { return Get()->GetInstanceNative(); }
		static Ref_old<VulkanDevice> GetCurrentDevice() { return Get()->GetDeviceNative(); }
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


