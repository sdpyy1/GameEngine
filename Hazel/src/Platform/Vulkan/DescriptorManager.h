#pragma once
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include <Hazel/Renderer/Image.h>
namespace Hazel
{
	class DescriptorManager
	{
	public:
		DescriptorManager(Ref<VulkanShader> shader) : m_Shader(shader) {
			m_BindImages.resize(Renderer::GetConfig().FramesInFlight);
			AdditionDescriptorSets.resize(Renderer::GetConfig().FramesInFlight);
		};

		void SetManagedDescriptorSet(uint32_t set)
		{
			AdditionDescriptorSets = m_Shader->GetDescriptorSet(set);
		}

		void SetInput(std::string name,Ref<Image2D> iamge,bool isInit);
		void SetInput(std::string name, Ref<UniformBufferSet> UboSet);
		void SetInput(std::string name, Ref<Texture2D> image, bool isInit);
		void SetInput(std::string name, Ref<StorageBufferSet> SBSet);
		void SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit);
		void SetInput(std::string name, Ref<ImageView> image);

		// 会输入到本地的AdditionDescriptorSets而不是Shader自动创建的,目前为Shader使用
		void SetInput2Addition(std::string name, Ref<Image2D> iamge, bool isInit);
		void SetInput2Addition(std::string name, Ref<UniformBufferSet> UboSet);
		void SetInput2Addition(std::string name, Ref<Texture2D> image, bool isInit);
		void SetInput2Addition(std::string name, Ref<StorageBufferSet> SBSet);
		void SetInput2Addition(std::string name, Ref<TextureCube> cubeMap, bool isInit);
		void SetInput2Addition(std::string name, Ref<ImageView> image);

		SetBindingKey GetBinding(const std::string& name)
		{
			return m_Shader->getSetAndBinding(name);
		}



	private:
		Ref<VulkanShader> m_Shader;
		std::vector<std::unordered_map<SetBindingKey, VkImage, SetBindingKeyHash>> m_BindImages; // 每个飞行帧的每个binding位置的图片缓存，为了避免重复Update
		std::vector<VkDescriptorSet> AdditionDescriptorSets;
	};
	
}

