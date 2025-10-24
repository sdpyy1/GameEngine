#include "hzpch.h"
#include "VulkanRenderPass.h"

#include "VulkanNativeCall.h"
#include "VulkanShader.h"
#include "VulkanContext.h"
#include "VulkanUniformBuffer.h"
#include "VulkanUniformBufferSet.h"
#include "VulkanStorageBuffer.h"
#include "VulkanStorageBufferSet.h"

#include "VulkanImage.h"
#include "VulkanTexture.h"

namespace Hazel {

	VulkanRenderPass::VulkanRenderPass(const RenderPassSpecification& spec)
		: m_Specification(spec)
	{
		HZ_CORE_VERIFY(spec.Pipeline);  // RenderPass must have Pipeline Info
		m_BindImages.resize(Renderer::GetConfig().FramesInFlight);
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}


	void VulkanRenderPass::SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding)
	{
		Renderer::Submit([=]() mutable {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput UBO Binding {1}", m_Specification.DebugName, Binding);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = UboSet->Get(i).As<VulkanUniformBuffer>()->GetVkBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = UboSet.As<VulkanUniformBufferSet>()->Get_PreSize();
				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
				descriptorWrites[0].dstBinding = Binding;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &UboSet->Get(i).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo();

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}});
	}

	void VulkanRenderPass::SetInput(Ref<Texture2D> texture, uint32_t Binding)
	{
		Renderer::Submit([=]() {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput texture Binding {1}", m_Specification.DebugName, Binding);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = texture.As<VulkanTexture2D>();

			for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
				VkDescriptorImageInfo imageInfo{};
				imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;
				imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;
				imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;

				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
				descriptorWrites[0].dstBinding = Binding;
				descriptorWrites[0].dstArrayElement = 0;
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pImageInfo = &imageInfo;

				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
			}
			});
	}

	Ref<Image2D> VulkanRenderPass::GetDepthOutput()
	{
		Ref<Framebuffer> framebuffer = m_Specification.Pipeline->GetSpecification().TargetFramebuffer;
		if (!framebuffer->HasDepthAttachment())
			return nullptr; // No depth output
		return framebuffer->GetDepthImage();
	}
	void VulkanRenderPass::SetInput(Ref<StorageBufferSet> bufferSet, uint32_t Binding, bool isInit)
	{
		Renderer::Submit([=]() mutable {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput StorageBuffer Binding {1}", m_Specification.DebugName, Binding);

			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			if (isInit) {
				for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
					auto vulkanBuffer = bufferSet->Get(i).As<VulkanStorageBuffer>();
					VkDescriptorBufferInfo bufferInfo{};
					bufferInfo.buffer = vulkanBuffer->GetVulkanBuffer();
					bufferInfo.offset = 0;
					bufferInfo.range = bufferSet.As<VulkanStorageBufferSet>()->Get_PreSize();

					// 描述符写入操作（结构与 UniformBuffer 完全一致，仅改类型）
					std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i]; // 目标描述符集（当前帧）
					descriptorWrites[0].dstBinding = Binding;         // 绑定点索引
					descriptorWrites[0].dstArrayElement = 0;          // 数组索引（非数组资源为 0）
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // 存储缓冲区类型（只读，根据需求可改）
					descriptorWrites[0].descriptorCount = 1;          // 资源数量
					descriptorWrites[0].pBufferInfo = &bufferInfo;    // 绑定缓冲区信息（直接使用上面的 bufferInfo）

					// 更新描述符集（与 UniformBuffer 调用方式一致）
					vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
			}
			else {
				auto vulkanBuffer = bufferSet->Get(Renderer::RT_GetCurrentFrameIndex()).As<VulkanStorageBuffer>();
				VkDescriptorBufferInfo bufferInfo{};
				bufferInfo.buffer = vulkanBuffer->GetVulkanBuffer();
				bufferInfo.offset = 0;
				bufferInfo.range = bufferSet.As<VulkanStorageBufferSet>()->Get_PreSize();

				// 描述符写入操作（结构与 UniformBuffer 完全一致，仅改类型）
				std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[Renderer::RT_GetCurrentFrameIndex()]; // 目标描述符集（当前帧）
				descriptorWrites[0].dstBinding = Binding;         // 绑定点索引
				descriptorWrites[0].dstArrayElement = 0;          // 数组索引（非数组资源为 0）
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // 存储缓冲区类型（只读，根据需求可改）
				descriptorWrites[0].descriptorCount = 1;          // 资源数量
				descriptorWrites[0].pBufferInfo = &bufferInfo;    // 绑定缓冲区信息（直接使用上面的 bufferInfo）

				// 更新描述符集（与 UniformBuffer 调用方式一致）
				vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			}
		});
	}

	void VulkanRenderPass::SetInput(Ref<Image2D> image, uint32_t Binding,bool isInit)
	{
		Renderer::Submit([=]() {
			//HZ_CORE_TRACE("RT: VulkanRenderPass [{0}]::SetInput Image2D Binding {1}", m_Specification.DebugName, Binding);
			VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
			auto vulkanTexture = image.As<VulkanImage2D>();

			if (isInit) {
				for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
					m_BindImages[i][Binding] = image.As<VulkanImage2D>()->GetVulkanImage();
					VkDescriptorImageInfo imageInfo{};
					imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;
					imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;
					imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;

					std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];
					descriptorWrites[0].dstBinding = Binding;
					descriptorWrites[0].dstArrayElement = 0;
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[0].descriptorCount = 1;
					descriptorWrites[0].pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
			}else {
				VkImage curImage = m_BindImages[Renderer::RT_GetCurrentFrameIndex()][Binding];
				// 图片被销毁或者更换了，需要更新绑定
				if (!curImage || curImage != image.As<VulkanImage2D>()->GetVulkanImage()) {
					HZ_CORE_INFO("VulkanRenderPass [{0}]::SetInput Image2D Binding {1} Update Image Bind", m_Specification.DebugName, Binding);
					m_BindImages[Renderer::RT_GetCurrentFrameIndex()][Binding] = image.As<VulkanImage2D>()->GetVulkanImage();
					VkDescriptorImageInfo imageInfo{};
					imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;
					imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;
					imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;

					std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
					descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[Renderer::RT_GetCurrentFrameIndex()];
					descriptorWrites[0].dstBinding = Binding;
					descriptorWrites[0].dstArrayElement = 0;
					descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
					descriptorWrites[0].descriptorCount = 1;
					descriptorWrites[0].pImageInfo = &imageInfo;

					vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
				}
			}
		});
	}
}
