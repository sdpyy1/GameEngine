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
			// ʵ�ʵ�ubo����
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = UboSet->Get(i).As<VulkanUniformBuffer>()->GetVkBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = UboSet.As<VulkanUniformBufferSet>()->Get_PreSize();
			// ����������д�����
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];  // Ҫ���µ���������
			descriptorWrites[0].dstBinding = Binding; // �󶨵㣬��Ӧ��ɫ��layout(binding = 0)
			descriptorWrites[0].dstArrayElement = 0; // ����ĵڼ���Ԫ�أ����ﲻ������������0
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1; // �󶨵���һ������ʱ������Ͳ���1��
			descriptorWrites[0].pBufferInfo = &UboSet->Get(i).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo(); // ��Դ��Ϣ

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}		
	void VulkanRenderPass::SetInput(Ref<Texture2D> texture, uint32_t Binding)
	{
		// ��ȡVulkan�豸
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		// ת��ΪVulkan������󣨼���Texture2D��ʵ����ΪVulkanTexture2D��
		auto vulkanTexture = texture.As<VulkanTexture2D>();

		// ��������֡���壨��֡ͬ������ƥ�䣩
		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			// ������������ͼ����Ϣ����������������ͼ�Ͳ��֣�
			VkDescriptorImageInfo imageInfo{};
			imageInfo.sampler = vulkanTexture->GetDescriptorInfoVulkan().sampler;               // ���������
			imageInfo.imageView = vulkanTexture->GetDescriptorInfoVulkan().imageView;           // ������ͼ
			imageInfo.imageLayout = vulkanTexture->GetDescriptorInfoVulkan().imageLayout;       // �����֣���ȷ��ΪVK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL��

			// ��������д�����
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = GetSpecification().Pipeline->GetShader().As<VulkanShader>()->GetDescriptorSet()[i];  // Ŀ����������
			descriptorWrites[0].dstBinding = Binding;                      // �󶨵㣨��Ӧ��ɫ���е�layout(binding = X)��
			descriptorWrites[0].dstArrayElement = 0;                       // ������������������Ϊ0��
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;  // �������������
			descriptorWrites[0].descriptorCount = 1;                       // ��������������������
			descriptorWrites[0].pImageInfo = &imageInfo;                   // ͼ����Դ��Ϣ�������������Ϣ��

			// ������������
			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
}
