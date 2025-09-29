#pragma once
#include "Vulkan/Vulkan.h"
#include <optional>
#include <vector>
struct GLFWwindow;

namespace Hazel {

#ifdef HZ_DEBUG
	const bool enableValidationLayers = true;
#else
	const bool enableValidationLayers = false;
#endif
	// ��֤��
	const std::vector<const char*> validationLayers = {
		"VK_LAYER_KHRONOS_validation"
	};

	// ��չ��
	const std::vector<const char*> deviceExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
	// �洢����������
	struct QueueFamilyIndices {
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		bool isComplete() {
			return graphicsFamily.has_value() && presentFamily.has_value();
		}
	};
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		// �������� VkVertexInputBindingDescription
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0; // ������ ����Ѷ������ݷŵ����������������Ҫ���������
			bindingDescription.stride = sizeof(Vertex); // ÿ��������ֽ���
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // ��ʾÿ�������Ӧһ��������Ŀ��ʵ����ʱ������һ��������

			return bindingDescription;
		}

		// ����������λ�á���ɫ�� VkVertexInputAttributeDescription
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0; // �������������bindingDescription.bindingһ��
			attributeDescriptions[0].location = 0; // λ��location����Ӧ������ɫ����layout(location = 0)
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3��ʽ
			attributeDescriptions[0].offset = offsetof(Vertex, pos); // λ�������ڽṹ���е�ƫ��

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
			return attributeDescriptions;
		}
	};
	const std::vector<Vertex> vertices = {
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};
	const std::vector<uint16_t> indices = {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
	};
	const int MAX_FRAMES_IN_FLIGHT = 2; // ֧����֡ͬʱ����

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};
	bool checkValidationLayerSupport();

	// ��ȡ��Ҫ����չ
	std::vector<const char*> getRequiredExtensions();

	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);

	VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
	
	bool isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window);

	VkImageView createImageView(VkDevice device,VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	std::vector<char> readShaderFromFile(const std::string& filename);
	VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
	VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
	uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void createImage(VkPhysicalDevice physicalDevice,VkDevice device, uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);

	void createBuffer(VkPhysicalDevice physicalDevice,VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

	VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool, VkDevice device);
	void transitionImageLayout(VkQueue graphicsQueue, VkCommandPool commandPool, VkDevice device,VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
	void endSingleTimeCommands(VkQueue graphicsQueue, VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer);
	void copyBufferToImage(VkQueue graphicsQueue, VkCommandPool commandPool, VkDevice device,VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);


	void copyBuffer(VkQueue graphicsQueue, VkCommandPool commandPool, VkDevice device,VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


}

