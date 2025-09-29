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
		void createDescriptorPool();
		// TODO: 只是先跑通
		void createDepthResources();
		void createRenderPass();
		void createDescriptorSetLayout();
		void createGraphicsPipeline();
		void createFramebuffers();

		// Temp: 装数据
		void createTextureImage();
		void createVertexBuffer();
		void createTextureSampler();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorSets();
		void createCommandBuffers();
		void createSyncObjects();
	protected:
		void recreateSwapChain();
		void cleanupSwapChain();
		void createImageViews();
		void updateUniformBuffer(uint32_t currentImage);
		void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
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
		VkDescriptorPool descriptorPool;

		// TODO: 
		VkDescriptorSetLayout descriptorSetLayout;
		VkRenderPass renderPass;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VkImage depthImage;
		VkDeviceMemory depthImageMemory;
		VkImageView depthImageView;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkImage textureImage;
		VkDeviceMemory textureImageMemory;
		VkImageView textureImageView;
		VkSampler textureSampler;
		VkBuffer vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer indexBuffer;
		VkDeviceMemory indexBufferMemory;
		std::vector<VkBuffer> uniformBuffers;
		std::vector<VkDeviceMemory> uniformBuffersMemory;
		std::vector<void*> uniformBuffersMapped;
		std::vector<VkDescriptorSet> descriptorSets;
		std::vector<VkCommandBuffer> commandBuffers;
		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		uint32_t currentFrame = 0;
		bool framebufferResized = false;

	};
}


