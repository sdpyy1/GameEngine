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
	}
	VulkanRenderPass::~VulkanRenderPass()
	{
	}


	void VulkanRenderPass::SetInput(Ref<UniformBufferSet> UboSet, uint32_t Binding)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			// 实际的ubo对象
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = UboSet->Get(i).As<VulkanUniformBuffer>()->GetVkBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = UboSet.As<VulkanUniformBufferSet>()->Get_PreSize();
			// 描述符集的写入操作
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];  // 要更新的描述符集
			descriptorWrites[0].dstBinding = Binding; // 绑定点，对应着色器layout(binding = 0)
			descriptorWrites[0].dstArrayElement = 0; // 数组的第几个元素，这里不是数组所以是0
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1; // 绑定点是一个数组时，这里就不是1了
			descriptorWrites[0].pBufferInfo = &UboSet->Get(i).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo(); // 资源信息

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}		
	void VulkanRenderPass::SetInput(Ref<Texture2D> texture, uint32_t Binding)
	{
		// 获取Vulkan设备
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		// 转换为Vulkan纹理对象（假设Texture2D的实现类为VulkanTexture2D）
		auto vulkanTexture = texture.As<VulkanTexture2D>();

		// 遍历所有帧缓冲（与帧同步数量匹配）
		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			// 纹理采样所需的图像信息（包含采样器、视图和布局）
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;               // 纹理采样器
			imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;           // 纹理视图
			imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;       // 纹理布局（需确保为VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL）

			// 描述符集写入操作
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];  // 目标描述符集
			descriptorWrites[0].dstBinding = Binding;                      // 绑定点（对应着色器中的layout(binding = X)）
			descriptorWrites[0].dstArrayElement = 0;                       // 数组索引（非数组则为0）
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // 纹理采样器类型
			descriptorWrites[0].descriptorCount = 1;                       // 描述符数量（单个纹理）
			descriptorWrites[0].pImageInfo = &imageInfo;                   // 图像资源信息（替代缓冲区信息）

			// 更新描述符集
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}
