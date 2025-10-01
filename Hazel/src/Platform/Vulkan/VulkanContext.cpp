#include "hzpch.h"
#include "VulkanContext.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include "VulkanUtils.h"
namespace Hazel {
	VulkanContext::VulkanContext(GLFWwindow* window)
		: window(window)
	{
		HZ_CORE_ASSERT(window, "Window handle is null!")
	}

	void VulkanContext::Init()
	{
		CheckVersion();
		HZ_CORE_INFO("Vulkan初始化开始！");
		createInstance(); 
		CreateDevice();
		createPipelineCache();
	}
	void VulkanContext::CreateDevice()
	{
		m_PhysicalDevice = VulkanPhysicalDevice::Select(m_Instance);
		HZ_CORE_INFO("Select PhysicalDevice: {0}",m_PhysicalDevice->getDeviceName());
		VkPhysicalDeviceFeatures enabledFeatures;
		memset(&enabledFeatures, 0, sizeof(VkPhysicalDeviceFeatures));
		enabledFeatures.samplerAnisotropy = true;
		enabledFeatures.wideLines = true;
		enabledFeatures.fillModeNonSolid = true;
		enabledFeatures.independentBlend = true;
		enabledFeatures.pipelineStatisticsQuery = true;
		enabledFeatures.shaderStorageImageReadWithoutFormat = true;
		m_Device = VulkanDevice::Create(m_PhysicalDevice, enabledFeatures);
		HZ_CORE_INFO("Create LogicDevice");
	}

	void VulkanContext::createInstance() {
		// App信息  可选
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "MyGameEngine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		// 创建信息
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		// 拓展
		auto extensions = VKUtils::getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// 验证层写入
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if (enableValidationLayers) {
			if (enableValidationLayers && !VKUtils::checkValidationLayerSupport()) {
				HZ_CORE_ASSERT("validation layers requested, but not available!");
			}
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
			debugCreateInfo = {};
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = VKUtils::debugCallback;
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}else {
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		// 创建实例
		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create instance!");
		}
		// 加载Debug Utils扩展函数
		VKUtils::VulkanLoadDebugUtilsExtensions(this->m_Instance);
		if (enableValidationLayers) {
			// 创建Debug Messenger
			VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
			debugCreateInfo = {};
			debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			debugCreateInfo.pfnUserCallback = VKUtils::debugCallback;
			if (VKUtils::CreateDebugUtilsMessengerEXT(m_Instance, &debugCreateInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}
		HZ_CORE_INFO("Create Vulkan instance");

		HZ_CORE_WARN("TODO: VMA impl");
	}

	
	void VulkanContext::SwapBuffers()
	{
		HZ_PROFILE_FUNCTION();

	}

	void VulkanContext::createPipelineCache()
	{
		// Pipeline Cache
		VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		VK_CHECK_RESULT(vkCreatePipelineCache(m_Device->GetVulkanDevice(), &pipelineCacheCreateInfo, nullptr, &m_PipelineCache));
		HZ_CORE_INFO("Pipeline Cache");

	}

	void VulkanContext::CheckVersion() {
		HZ_CORE_ASSERT(glfwVulkanSupported(), "GLFW must support Vulkan!");
		if (!VKUtils::CheckDriverAPIVersionSupport(VK_API_VERSION_1_2))
		{
#ifdef HZ_PLATFORM_WINDOWS
			MessageBox(nullptr, L"Incompatible Vulkan driver version.\nUpdate your GPU drivers!", L"Hazel Error", MB_OK | MB_ICONERROR);
#else
			HZ_CORE_ERROR("Incompatible Vulkan driver version.\nUpdate your GPU drivers!");
#endif
			HZ_CORE_ERROR(false);
		}
	}

}
