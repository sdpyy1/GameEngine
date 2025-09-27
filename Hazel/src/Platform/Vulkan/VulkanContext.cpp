#include "hzpch.h"
#include "VulkanContext.h"
#include "Vulkan/Vulkan.h"
namespace Hazel {

	VulkanContext::VulkanContext(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		HZ_CORE_ASSERT(windowHandle, "Window handle is null!")

	}

	void VulkanContext::Init()
	{
		HZ_PROFILE_FUNCTION();
		HZ_CORE_INFO("VulkanContext::Create");

	}

	void VulkanContext::SwapBuffers()
	{
		HZ_PROFILE_FUNCTION();

	}
}
