#pragma once
#include "Hazel/Renderer/GraphicsContext.h"
#include "VulkanUtils.h"

struct GLFWwindow;

namespace Hazel {

	class VulkanContext : public RenderContext
	{
	public:
		VulkanContext(GLFWwindow* windowHandle);
		virtual void Init() override;
		virtual void SwapBuffers() override;
	protected:
		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createImageViewsForSwapChain();
		void createCommandPool();
	private:
		VkInstance instance;
		GLFWwindow* window = nullptr;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR surface;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice device;
		VkQueue graphicsQueue;
		VkQueue presentQueue;
		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent; // Height And Width of SwapChain
		std::vector<VkImageView> swapChainImageViews;
		VkCommandPool commandPool;

	};
}


