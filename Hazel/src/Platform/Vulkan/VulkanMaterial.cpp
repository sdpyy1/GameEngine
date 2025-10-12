#include "hzpch.h"
#include "VulkanMaterial.h"
#include <Hazel/Renderer/Shader.h>
#include "VulkanContext.h"
#include "VulkanTexture.h"
namespace Hazel {
	VulkanMaterial::VulkanMaterial(const Ref<Shader>& shader, const std::string& name)
		: m_Shader(shader.As<VulkanShader>()), m_Name(name)
	{
		m_Device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
	}

	void VulkanMaterial::UpdateDescriptorSet() {
		// 1. ѡ��ǰҪ�󶨵���ͼ�������û����ã�������Ĭ�ϣ�
		auto albedo = AlbedoTexture;
		auto normal = NormalTexture;
		auto metalness =MetalnessTexture;
		auto roughness = RoughnessTexture;

		// 2. ׼����ͼ�� ImageInfo����ϲ�������������ͼ�Դ���������
		auto getImageInfo = [](Ref<Texture2D> tex) -> VkDescriptorImageInfo {
			VkDescriptorImageInfo info{};
			info.sampler = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan().sampler; // ���� Texture2D ���� GetSampler ����
			info.imageView = tex.As<VulkanTexture2D>()->GetDescriptorInfoVulkan().imageView; // ���� Texture2D ���� GetImageView ����
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // ��ͼ����ǰת�����ò���
			return info;
			};

		std::array<VkDescriptorImageInfo, 4> imageInfos = {
			getImageInfo(albedo),    // binding 1: u_AlbedoTexture
			getImageInfo(normal),    // binding 2: u_NormalTexture
			getImageInfo(metalness), // binding 3: u_MetalnessTexture
			getImageInfo(roughness)  // binding 4: u_RoughnessTexture
		};

		// 3. ׼��������д�������д�� Shader �� Set = 0 �� binding 1~4��
		std::array<VkWriteDescriptorSet, 4> writeSets{};
		for (size_t i = 0; i < 4; i++) {
			writeSets[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeSets[i].dstBinding = static_cast<uint32_t>(i) + 1; // binding = 1~4���� Shader ��Ӧ��
			writeSets[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			writeSets[i].descriptorCount = 1;
			writeSets[i].pImageInfo = &imageInfos[i];
		}

		// 4. Ϊ Shader ������֡�� Set = 0 ������ͼ����֡����ʱÿ��֡��Ҫ���£�
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		for (uint32_t frame = 0; frame < framesInFlight; frame++) {
			// ��ȡ Shader �� Set = 0 ������������ǰ֡��
			VkDescriptorSet shaderSet = m_Shader->GetDescriptorSet()[frame];

			// Ϊ��ǰ֡�� Set ���� dstSet
			for (auto& write : writeSets) {
				write.dstSet = shaderSet;
			}

			// �������µ�ǰ֡�� 4 �� binding
			vkUpdateDescriptorSets(
				m_Device,
				static_cast<uint32_t>(writeSets.size()),
				writeSets.data(),
				0, nullptr
			);
		}
	}
	void VulkanMaterial::SetNormalTexture(Ref<Texture2D> texture)
	{
		NormalTexture = texture;
	}
	void VulkanMaterial::SetMetalnessTexture(Ref<Texture2D> texture)
	{
		MetalnessTexture = texture;
	}
	void VulkanMaterial::SetRoughnessTexture(Ref<Texture2D> texture)
	{
		RoughnessTexture = texture;
	}
	void VulkanMaterial::SetAlbedoTexture(Ref<Texture2D> texture)
	{
		AlbedoTexture = texture;
	}
	VulkanMaterial::~VulkanMaterial()
	{
	}
}
