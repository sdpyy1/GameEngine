#include "hzpch.h"
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
namespace Hazel {
	VulkanShader::VulkanShader(const std::string& path, bool forceCompile, bool disableOptimization)
		: m_AssetPath(path), m_DisableOptimization(disableOptimization)
	{
		// TODO: This should be more "general"   直接截取文件名作为Shader名字
		size_t found = path.find_last_of("/\\");
		m_Name = found != std::string::npos ? path.substr(found + 1) : path;
		found = m_Name.find_last_of('.');
		m_Name = found != std::string::npos ? m_Name.substr(0, found) : m_Name;

		Reload(forceCompile);
	}

	void VulkanShader::Reload(bool forceCompile)
	{
		//Renderer::Submit([instance = Ref(this), forceCompile]() mutable
		//	{
		//		HZ_CORE_INFO("Create Shader");
		//		instance->RT_Reload(forceCompile);
		//	});

		RT_Reload(forceCompile);

	}

	static std::vector<char> readFile(const std::filesystem::path& filename) {  // 参数改为std::filesystem::path
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filename.string());
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		return buffer;
	}
	void VulkanShader::RT_Reload(const bool forceCompile)
	{
//#if HZ_HAS_SHADER_COMPILER 
		//if (!VulkanShaderCompiler::TryRecompile(this))
		//{
		//	HZ_CORE_FATAL("Failed to recompile shader!");
		//}
//#endif

		// TODO: 先实现最简单的Shader载入功能
		auto code = readFile(m_AssetPath);
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		
		if (vkCreateShaderModule(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}


	}


	
	VulkanShader::~VulkanShader()
	{
	}
}
