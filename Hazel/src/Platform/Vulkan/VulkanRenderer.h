#pragma once
#include "Hazel/Renderer/RendererAPI.h"
#include "Vulkan.h"
namespace Hazel {
	class VulkanRenderer : public RendererAPI
	{
	public:
		virtual void Init() override;
		virtual void Shutdown() override;

		virtual RendererCapabilities& GetCapabilities() override;

		virtual void BeginFrame() override;
		virtual void EndFrame() override;

		static VkDescriptorSet RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo);



		// TODO:²»±¨´í
		virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {};
		virtual void SetClearColor(const glm::vec4& color) {};
		virtual void Clear() {};

		virtual void DrawIndexed(const Ref_old<VertexArray>& vertexArray, uint32_t indexCount =0) {};
		virtual void DrawLines(const Ref_old<VertexArray>& vertexArray, uint32_t vertexCount) {};

		virtual void SetLineWidth(float width) {};
	};

	namespace Utils {

		void InsertImageMemoryBarrier(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

	}
}

