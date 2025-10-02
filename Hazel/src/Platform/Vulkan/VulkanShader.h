#pragma once
#include"Hazel/Renderer/Shader.h"
#include <vulkan/vulkan.h>
namespace Hazel {

	class VulkanShader : public Shader
	{
	public:		
		VulkanShader() = default;
		VulkanShader(const std::string& path, bool forceCompile, bool disableOptimization);

		void Reload(bool forceCompile = false) override;
		void RT_Reload(bool forceCompile) override;
		virtual ~VulkanShader();
		void Release();
		virtual const std::string& GetName() const override { return m_Name; }

	private:
		std::string m_Name;
		bool m_DisableOptimization = false;
		std::filesystem::path m_AssetPath;
		VkShaderModule shaderModule;


	public:
		// TODO:Tmp add
		virtual void Bind() const{};
		virtual void Unbind() const{};

		virtual void SetInt(const std::string& name, int value){};
		virtual void SetIntArray(const std::string& name, int* values, uint32_t count){};
		virtual void SetFloat(const std::string& name, float value){};
		virtual void SetFloat2(const std::string& name, const glm::vec2& value){};
		virtual void SetFloat3(const std::string& name, const glm::vec3& value){};
		virtual void SetFloat4(const std::string& name, const glm::vec4& value){};
		virtual void SetMat4(const std::string& name, const glm::mat4& value){};
	};
}
