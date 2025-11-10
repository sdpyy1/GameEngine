#include "hzpch.h"
#include "VulkanShaderCompiler.h"
#include "VulkanContext.h"
#include <shaderc/shaderc.hpp>

namespace Hazel
{ 
	VulkanShaderCompiler::VulkanShaderCompiler(Ref<VulkanShader> shader) : m_Shader(shader){}

	VkShaderModule VulkanShaderCompiler::CompileShaderModule()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		std::string glslPath = "Assets/Shader/DirShadowMap.glsl";

		// 2️⃣ 读取 GLSL 文件
		std::ifstream file(glslPath, std::ios::binary);
		if (!file)
			throw std::runtime_error("Failed to open shader: " + glslPath);
		std::string source((std::istreambuf_iterator<char>(file)),
			std::istreambuf_iterator<char>());

		// 3️⃣ 设置 shaderc 编译选项
		shaderc::CompileOptions options;
		options.SetOptimizationLevel(shaderc_optimization_level_performance);
		options.SetGenerateDebugInfo();

		// 直接写死宏定义
		options.AddMacroDefinition("VERTEX_SHADER", "1");

		// 4️⃣ 编译 GLSL → SPIR-V
		shaderc::Compiler compiler;
		shaderc::SpvCompilationResult module =
			compiler.CompileGlslToSpv(source, shaderc_vertex_shader, glslPath.c_str(), options);

		if (module.GetCompilationStatus() != shaderc_compilation_status_success)
			throw std::runtime_error("Shader compile failed: " + module.GetErrorMessage());

		std::vector<uint32_t> spirv(module.cbegin(), module.cend());

		// 5️⃣ 创建 Vulkan ShaderModule
		VkShaderModuleCreateInfo ci{};
		ci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		ci.codeSize = spirv.size() * sizeof(uint32_t);
		ci.pCode = spirv.data();

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &ci, nullptr, &shaderModule) != VK_SUCCESS)
			throw std::runtime_error("Failed to create VkShaderModule");

		return shaderModule;
	}

}

