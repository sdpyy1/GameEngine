#pragma once
#include "VulkanShader.h"
namespace Hazel
{
	// TODO: 待整合，目前的资源描述符管理逻辑清晰，但是普通Shader和ComputerShader分别写了逻辑，应该合并
	class DescriptorManager
	{
	public:
		DescriptorManager(Ref<VulkanShader> shader) : m_Shader(shader) {};





	private:
		Ref<VulkanShader> m_Shader;

	};
	
}

