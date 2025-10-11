#include "hzpch.h"
#include "VulkanRenderer.h"
#include <Hazel/Renderer/RendererCapabilities.h>
#include "Vulkan.h"
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
#include "VulkanUtils.h"
#include <imgui.h>
#include <Hazel/Renderer/IndexBuffer.h>
#include "VulkanNativeCall.h"
#include "VulkanFramebuffer.h"
#include "VulkanPipeline.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanRenderCommandBuffer.h"
#include <glm/gtc/type_ptr.hpp>
#include "VulkanRenderPass.h"

namespace Hazel {
	struct VulkanRendererData
	{
		RendererCapabilities RenderCaps;

		Ref<Texture2D> BRDFLut;

		Ref<VertexBuffer> QuadVertexBuffer;
		Ref<IndexBuffer> QuadIndexBuffer;
		//VulkanShader::ShaderMaterialDescriptorSet QuadDescriptorSet;

		//std::unordered_map<SceneRenderer*, std::vector<VulkanShader::ShaderMaterialDescriptorSet>> RendererDescriptorSet;
		VkDescriptorSet ActiveRendererDescriptorSet = nullptr;
		std::vector<VkDescriptorPool> DescriptorPools;
		VkDescriptorPool MaterialDescriptorPool;
		std::vector<uint32_t> DescriptorPoolAllocationCount;

		// UniformBufferSet -> Shader Hash -> Frame -> WriteDescriptor
		std::unordered_map<UniformBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> UniformBufferWriteDescriptorCache;
		std::unordered_map<StorageBufferSet*, std::unordered_map<uint64_t, std::vector<std::vector<VkWriteDescriptorSet>>>> StorageBufferWriteDescriptorCache;

		// Default samplers
		VkSampler SamplerClamp = nullptr;
		VkSampler SamplerPoint = nullptr;

		int32_t SelectedDrawCall = -1;
		int32_t DrawCallCount = 0;
	};
	static VulkanRendererData* s_Data = nullptr;

	namespace Utils {

		static const char* VulkanVendorIDToString(uint32_t vendorID)
		{
			switch (vendorID)
			{
			case 0x10DE: return "NVIDIA";
			case 0x1002: return "AMD";
			case 0x8086: return "INTEL";
			case 0x13B5: return "ARM";
			}
			return "Unknown";
		}

	}
	VkDescriptorSet VulkanRenderer::RT_AllocateDescriptorSet(VkDescriptorSetAllocateInfo& allocInfo)
	{
		uint32_t bufferIndex = Renderer::RT_GetCurrentFrameIndex();
		allocInfo.descriptorPool = s_Data->DescriptorPools[bufferIndex];
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		VkDescriptorSet result;
		VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, &result));
		s_Data->DescriptorPoolAllocationCount[bufferIndex] += allocInfo.descriptorSetCount;
		return result;
	}
	void VulkanRenderer::Init()
	{
		// 初始化一些资源
		s_Data = new VulkanRendererData();
		const auto& config = Renderer::GetConfig();
		// 为并发帧都创建DescriptorPool
		s_Data->DescriptorPools.resize(config.FramesInFlight);
		s_Data->DescriptorPoolAllocationCount.resize(config.FramesInFlight);
		auto& caps = s_Data->RenderCaps;
		auto& properties = VulkanContext::GetCurrentDevice()->GetPhysicalDevice()->GetProperties();
		caps.Vendor = Utils::VulkanVendorIDToString(properties.vendorID);
		caps.Device = properties.deviceName;
		caps.Version = std::to_string(properties.driverVersion);
		HZ_CORE_INFO_TAG("Renderer", "GPU Info:");
		HZ_CORE_INFO_TAG("Renderer", "  Vendor: {0}", caps.Vendor);
		HZ_CORE_INFO_TAG("Renderer", "  Device: {0}", caps.Device);
		HZ_CORE_INFO_TAG("Renderer", "  Version: {0}", caps.Version);
		// 创建 descriptor pools
		Renderer::Submit([]() mutable
			{
				VkDescriptorPoolSize pool_sizes[] =
				{
					{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
					{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
					{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
					{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
				};
				VkDescriptorPoolCreateInfo pool_info = {};
				pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
				pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
				pool_info.maxSets = 100000;
				pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
				pool_info.pPoolSizes = pool_sizes;
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
				for (uint32_t i = 0; i < framesInFlight; i++)
				{
					VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->DescriptorPools[i]));
					s_Data->DescriptorPoolAllocationCount[i] = 0;
				}

				VK_CHECK_RESULT(vkCreateDescriptorPool(device, &pool_info, nullptr, &s_Data->MaterialDescriptorPool));
			});

		// 提前把全屏正方形顶点数据创建好
		float x = -1;
		float y = -1;
		float width = 2, height = 2;
		struct QuadVertex
		{
			glm::vec3 Position;
			glm::vec2 TexCoord;
		};

		QuadVertex* data = new QuadVertex[4];

		data[0].Position = glm::vec3(x, y, 0.0f);
		data[0].TexCoord = glm::vec2(0, 0);

		data[1].Position = glm::vec3(x + width, y, 0.0f);
		data[1].TexCoord = glm::vec2(1, 0);

		data[2].Position = glm::vec3(x + width, y + height, 0.0f);
		data[2].TexCoord = glm::vec2(1, 1);

		data[3].Position = glm::vec3(x, y + height, 0.0f);
		data[3].TexCoord = glm::vec2(0, 1);

		s_Data->QuadVertexBuffer = VertexBuffer::Create(data, 4 * sizeof(QuadVertex));
		uint32_t indices[6] = { 0, 1, 2, 2, 3, 0, };
		s_Data->QuadIndexBuffer = IndexBuffer::Create(indices, 6 * sizeof(uint32_t));

	}
	void VulkanRenderer::Shutdown()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		vkDeviceWaitIdle(device);

		if (s_Data->SamplerPoint)
		{
			Vulkan::DestroySampler(s_Data->SamplerPoint);
			s_Data->SamplerPoint = nullptr;
		}

		if (s_Data->SamplerClamp)
		{
			Vulkan::DestroySampler(s_Data->SamplerClamp);
			s_Data->SamplerClamp = nullptr;
		}

#if HZ_HAS_SHADER_COMPILER
		VulkanShaderCompiler::ClearUniformBuffers();
#endif
		delete s_Data;
	}


	RendererCapabilities& VulkanRenderer::GetCapabilities()
	{
		return s_Data->RenderCaps;
	}
	void VulkanRenderer::BeginFrame()
	{
		Renderer::Submit([]()
			{
				VulkanSwapChain& swapChain = Application::Get().GetWindow()->GetSwapChain();
				// 清空命令缓冲区、获取下一帧图片索引
				swapChain.BeginFrame();
				// Reset descriptor pools here
				VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
				uint32_t bufferIndex = swapChain.GetCurrentBufferIndex();

				// TODO:这个Pool 目前我还没用到，目前每个Shader一个Pool
				//vkResetDescriptorPool(device, s_Data->DescriptorPools[bufferIndex], 0);
				//memset(s_Data->DescriptorPoolAllocationCount.data(), 0, s_Data->DescriptorPoolAllocationCount.size() * sizeof(uint32_t));

				s_Data->DrawCallCount = 0;
			});
	}
	void VulkanRenderer::BindVertData(Ref<RenderCommandBuffer> commandBuffer, Ref<VertexBuffer> testVertexBuffer) {
		
		Renderer::Submit([commandBuffer,testVertexBuffer] {
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer vkCommandBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
			VkDeviceSize offsets[] = { 0 };
			VkBuffer vertexBuffers[] = { testVertexBuffer.As<VulkanVertexBuffer>()->GetVulkanBuffer() };
			vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
			});

	}
	void VulkanRenderer::BindIndexDataAndDraw(Ref<RenderCommandBuffer> commandBuffer, Ref<IndexBuffer> indexBuffer)
	{
		Renderer::Submit([commandBuffer,indexBuffer] {
			uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
			VkCommandBuffer vkCommandBuffer = commandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();
			vkCmdBindIndexBuffer(vkCommandBuffer, indexBuffer.As<VulkanIndexBuffer>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT32);
			vkCmdDrawIndexed(vkCommandBuffer, indexBuffer->GetCount(), 1, 0, 0, 0);
		});

	}
	void VulkanRenderer::EndFrame()
	{
		Renderer::Submit([]() {
			Ref<WindowsWindow> m_Window = Application::Get().GetWindow();
			// 提交命令缓冲区、呈现图片
			m_Window->SwapBuffers();
		});
#if 0

		Renderer::Submit([]()
			{
				VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer));
				commandBuffer = nullptr;
			});
#endif
	}

	void VulkanRenderer::BeginRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer, Ref<RenderPass> renderPass, bool explicitClear)
	{
		Renderer::Submit([renderCommandBuffer, renderPass, explicitClear]()
			{
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

				VkDebugUtilsLabelEXT debugLabel{};
				debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
				memcpy(&debugLabel.color, glm::value_ptr(renderPass->GetSpecification().MarkerColor), sizeof(float) * 4);
				debugLabel.pLabelName = renderPass->GetSpecification().DebugName.c_str();
				fpCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);

				auto fb = renderPass->GetSpecification().Pipeline->GetSpecification().TargetFramebuffer;
				Ref<VulkanFramebuffer> framebuffer = fb.As<VulkanFramebuffer>();
				const auto& fbSpec = framebuffer->GetSpecification();

				uint32_t width = framebuffer->GetWidth();
				uint32_t height = framebuffer->GetHeight();

				VkViewport viewport = {};
				viewport.minDepth = 0.0f;
				viewport.maxDepth = 1.0f;

				VkRenderPassBeginInfo renderPassBeginInfo = {};
				renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
				renderPassBeginInfo.pNext = nullptr;
				renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
				renderPassBeginInfo.renderArea.offset.x = 0;
				renderPassBeginInfo.renderArea.offset.y = 0;
				renderPassBeginInfo.renderArea.extent.width = width;
				renderPassBeginInfo.renderArea.extent.height = height;
				if (framebuffer->GetSpecification().SwapChainTarget)
				{
					VulkanSwapChain& swapChain = Application::Get().GetWindow()->GetSwapChain();
					width = swapChain.GetWidth();
					height = swapChain.GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = swapChain.GetCurrentFramebuffer();

					viewport.x = 0.0f;
					viewport.y = (float)height;
					viewport.width = (float)width;
					viewport.height = -(float)height;
				}
				else
				{
					width = framebuffer->GetWidth();
					height = framebuffer->GetHeight();
					renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
					renderPassBeginInfo.pNext = nullptr;
					renderPassBeginInfo.renderPass = framebuffer->GetRenderPass();
					renderPassBeginInfo.renderArea.offset.x = 0;
					renderPassBeginInfo.renderArea.offset.y = 0;
					renderPassBeginInfo.renderArea.extent.width = width;
					renderPassBeginInfo.renderArea.extent.height = height;
					renderPassBeginInfo.framebuffer = framebuffer->GetVulkanFramebuffer();

					viewport.x = 0.0f;
					viewport.y = 0.0f;
					viewport.width = (float)width;
					viewport.height = (float)height;
				}

				// TODO: Does our framebuffer have a depth attachment?
				const auto& clearValues = framebuffer->GetVulkanClearValues();
				renderPassBeginInfo.clearValueCount = (uint32_t)clearValues.size();
				renderPassBeginInfo.pClearValues = clearValues.data();

				vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

				if (explicitClear)
				{
					const uint32_t colorAttachmentCount = (uint32_t)framebuffer->GetColorAttachmentCount();
					const uint32_t totalAttachmentCount = colorAttachmentCount + (framebuffer->HasDepthAttachment() ? 1 : 0);
					HZ_CORE_ASSERT(clearValues.size() == totalAttachmentCount);

					std::vector<VkClearAttachment> attachments(totalAttachmentCount);
					std::vector<VkClearRect> clearRects(totalAttachmentCount);
					for (uint32_t i = 0; i < colorAttachmentCount; i++)
					{
						attachments[i].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
						attachments[i].colorAttachment = i;
						attachments[i].clearValue = clearValues[i];

						clearRects[i].rect.offset = { (int32_t)0, (int32_t)0 };
						clearRects[i].rect.extent = { width, height };
						clearRects[i].baseArrayLayer = 0;
						clearRects[i].layerCount = 1;
					}

					if (framebuffer->HasDepthAttachment())
					{
						attachments[colorAttachmentCount].aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT /*| VK_IMAGE_ASPECT_STENCIL_BIT*/;
						attachments[colorAttachmentCount].clearValue = clearValues[colorAttachmentCount];
						clearRects[colorAttachmentCount].rect.offset = { (int32_t)0, (int32_t)0 };
						clearRects[colorAttachmentCount].rect.extent = { width, height };
						clearRects[colorAttachmentCount].baseArrayLayer = 0;
						clearRects[colorAttachmentCount].layerCount = 1;
					}

					vkCmdClearAttachments(commandBuffer, totalAttachmentCount, attachments.data(), totalAttachmentCount, clearRects.data());

				}

				// Update dynamic viewport state
				vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

				// Update dynamic scissor state
				VkRect2D scissor = {};
				scissor.extent.width = width;
				scissor.extent.height = height;
				scissor.offset.x = 0;
				scissor.offset.y = 0;
				vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

				// TODO: automatic layout transitions for input resources

				// Bind Vulkan Pipeline
				Ref<VulkanPipeline> vulkanPipeline = renderPass->GetSpecification().Pipeline.As<VulkanPipeline>();
				VkPipeline vPipeline = vulkanPipeline->GetVulkanPipeline();
				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vPipeline);

				if (vulkanPipeline->IsDynamicLineWidth())
					vkCmdSetLineWidth(commandBuffer, vulkanPipeline->GetSpecification().LineWidth);

				// Bind input descriptors (starting from set 1, set 0 is for per-draw)
				Ref<VulkanRenderPass> vulkanRenderPass = renderPass.As<VulkanRenderPass>();
				vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, renderPass->GetPipeline().As<VulkanPipeline>()->GetVulkanPipelineLayout(), 0, 1, &renderPass->GetPipeline()->GetShader().As<VulkanShader>()->GetDescriptorSet()[frameIndex], 0, nullptr);
				/*vulkanRenderPass->Prepare();
				if (vulkanRenderPass->HasDescriptorSets())
				{
					const auto& descriptorSets = vulkanRenderPass->GetDescriptorSets(frameIndex);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vulkanPipeline->GetVulkanPipelineLayout(), vulkanRenderPass->GetFirstSetIndex(), (uint32_t)descriptorSets.size(), descriptorSets.data(), 0, nullptr);
				}*/
			});
	
	}	
	void VulkanRenderer::EndRenderPass(Ref<RenderCommandBuffer> renderCommandBuffer)
	{
		Renderer::Submit([renderCommandBuffer]()
			{
				uint32_t frameIndex = Renderer::RT_GetCurrentFrameIndex();
				VkCommandBuffer commandBuffer = renderCommandBuffer.As<VulkanRenderCommandBuffer>()->GetActiveCommandBuffer();

				vkCmdEndRenderPass(commandBuffer);
				fpCmdEndDebugUtilsLabelEXT(commandBuffer);
			});
	}
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
			VkImageSubresourceRange subresourceRange)
		{
			VkImageMemoryBarrier imageMemoryBarrier{};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

			imageMemoryBarrier.srcAccessMask = srcAccessMask;
			imageMemoryBarrier.dstAccessMask = dstAccessMask;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkImageSubresourceRange subresourceRange,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			// Create an image barrier object
			VkImageMemoryBarrier imageMemoryBarrier = {};
			imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			imageMemoryBarrier.oldLayout = oldImageLayout;
			imageMemoryBarrier.newLayout = newImageLayout;
			imageMemoryBarrier.image = image;
			imageMemoryBarrier.subresourceRange = subresourceRange;

			// Source layouts (old)
			// Source access mask controls actions that have to be finished on the old layout
			// before it will be transitioned to the new layout
			switch (oldImageLayout)
			{
			case VK_IMAGE_LAYOUT_UNDEFINED:
				// Image layout is undefined (or does not matter)
				// Only valid as initial layout
				// No flags required, listed only for completeness
				imageMemoryBarrier.srcAccessMask = 0;
				break;

			case VK_IMAGE_LAYOUT_PREINITIALIZED:
				// Image is preinitialized
				// Only valid as initial layout for linear images, preserves memory contents
				// Make sure host writes have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image is a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image is a depth/stencil attachment
				// Make sure any writes to the depth/stencil buffer have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image is a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image is a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image is read by a shader
				// Make sure any shader reads from the image have been finished
				imageMemoryBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Target layouts (new)
			// Destination access mask controls the dependency for the new image layout
			switch (newImageLayout)
			{
			case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
				// Image will be used as a transfer destination
				// Make sure any writes to the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
				// Image will be used as a transfer source
				// Make sure any reads from the image have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
				break;

			case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
				// Image will be used as a color attachment
				// Make sure any writes to the color buffer have been finished
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
				// Image layout will be used as a depth/stencil attachment
				// Make sure any writes to depth/stencil buffer have been finished
				imageMemoryBarrier.dstAccessMask = imageMemoryBarrier.dstAccessMask | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
				break;

			case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
				// Image will be read in a shader (sampler, input attachment)
				// Make sure any writes to the image have been finished
				if (imageMemoryBarrier.srcAccessMask == 0)
				{
					imageMemoryBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
				}
				imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
				break;
			default:
				// Other source layouts aren't handled (yet)
				break;
			}

			// Put barrier inside setup command buffer
			vkCmdPipelineBarrier(
				cmdbuffer,
				srcStageMask,
				dstStageMask,
				0,
				0, nullptr,
				0, nullptr,
				1, &imageMemoryBarrier);
		}

		void SetImageLayout(
			VkCommandBuffer cmdbuffer,
			VkImage image,
			VkImageAspectFlags aspectMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask)
		{
			VkImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = aspectMask;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = 1;
			SetImageLayout(cmdbuffer, image, oldImageLayout, newImageLayout, subresourceRange, srcStageMask, dstStageMask);
		}

	}

}
