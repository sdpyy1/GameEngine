#include "hzpch.h"
#include "VulkanContext.h"
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include "VulkanUtils.h"
#include "VulkanAllocator.h"
namespace Hazel {
#if defined(HZ_DEBUG) || defined(HZ_RELEASE)
	static bool s_Validation = true;
#else
	static bool s_Validation = false; // Let's leave this on for now...
#endif
	VulkanContext::VulkanContext(void* window)
		: window(window)
	{
		HZ_CORE_ASSERT(window, "Window handle is null!")
	}

	void VulkanContext::Init()
	{
		CheckVersion();
		HZ_CORE_INFO("Vulkan��ʼ����ʼ��");
		createInstance(); 
		CreateDevice();
		createPipelineCache();
		VulkanAllocator::Init(m_Device,m_Instance);
		// ����Debug Utils��չ����
		VKUtils::VulkanLoadDebugUtilsExtensions(m_Instance, m_Device->GetVulkanDevice());
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
		m_Device = VulkanDevice::Create_old(m_PhysicalDevice, enabledFeatures);
		HZ_CORE_INFO("Create LogicDevice");
	}

	void VulkanContext::createInstance() {
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
				// Application Info
				/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hazel";
		appInfo.pEngineName = "Hazel";
		appInfo.apiVersion = VK_API_VERSION_1_2;

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Extensions and Validation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO(Emily): GLFW can handle this for us
#ifdef HZ_PLATFORM_WINDOWS
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_win32_surface"
#elif defined(HZ_PLATFORM_LINUX)
#define VK_KHR_WIN32_SURFACE_EXTENSION_NAME "VK_KHR_xcb_surface"
#endif
		std::vector<const char*> instanceExtensions = { VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME };
		instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME); // Very little performance hit, can be used in Release.
		if (s_Validation)
		{
			instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
			instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
		}

		VkValidationFeatureEnableEXT enables[] = { VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT };
		VkValidationFeaturesEXT features = {};
		features.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
		features.enabledValidationFeatureCount = 1;
		features.pEnabledValidationFeatures = enables;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pNext = nullptr; // &features;
		instanceCreateInfo.pApplicationInfo = &appInfo;
		instanceCreateInfo.enabledExtensionCount = (uint32_t)instanceExtensions.size();
		instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions.data();

		// TODO: Extract all validation into separate class
		if (s_Validation)
		{
			const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
			// Check if this layer is available at instance level
			uint32_t instanceLayerCount;
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
			std::vector<VkLayerProperties> instanceLayerProperties(instanceLayerCount);
			vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayerProperties.data());
			bool validationLayerPresent = false;
			HZ_CORE_INFO_TAG("Renderer", "Vulkan Instance Layers:");
			for (const VkLayerProperties& layer : instanceLayerProperties)
			{
				HZ_CORE_INFO_TAG("Renderer", "  {0}", layer.layerName);
				if (strcmp(layer.layerName, validationLayerName) == 0)
				{
					validationLayerPresent = true;
					break;
				}
			}
			if (validationLayerPresent)
			{
				instanceCreateInfo.ppEnabledLayerNames = &validationLayerName;
				instanceCreateInfo.enabledLayerCount = 1;
			}
			else
			{
				HZ_CORE_ERROR_TAG("Renderer", "Validation layer VK_LAYER_KHRONOS_validation not present, validation is disabled");
			}
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// Instance and Surface Creation
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		vkCreateInstance(&instanceCreateInfo, nullptr, &m_Instance);

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
