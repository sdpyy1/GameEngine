#pragma once
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include <Hazel/Renderer/Image.h>
namespace Hazel
{
	// TODO: 待整合，目前的资源描述符管理逻辑清晰，但是普通Shader和ComputerShader分别写了逻辑，应该合并
	class DescriptorManager
	{
	public:
		DescriptorManager(Ref<VulkanShader> shader) : m_Shader(shader) {
			m_BindImages.resize(Renderer::GetConfig().FramesInFlight);
		};



		void SetInput(std::string name,Ref<Image2D> iamge,bool isInit);
		void SetInput(std::string name, Ref<UniformBufferSet> UboSet);
		void SetInput(std::string name, Ref<Texture2D> image, bool isInit);
		void SetInput(std::string name, Ref<StorageBufferSet> SBSet);
		void SetInput(std::string name, Ref<TextureCube> cubeMap, bool isInit);



		SetBindingKey GetBinding(const std::string& name)
		{
			return m_Shader->getSetAndBinding(name);
		}



	private:
		Ref<VulkanShader> m_Shader;
		std::vector<std::unordered_map<SetBindingKey, VkImage, SetBindingKeyHash>> m_BindImages; // 每个飞行帧的每个binding位置的图片缓存，为了避免重复Update

	};
	
}

