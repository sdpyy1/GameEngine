#pragma once
#include "VulkanShader.h"
namespace Hazel
{
	class VulkanShaderCompiler
	{

	public:
		VulkanShaderCompiler(Ref<VulkanShader> shader);
		VkShaderModule CompileShaderModule();



	private:
		Ref<VulkanShader> m_Shader;
	};


}
