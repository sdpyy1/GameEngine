#include "hzpch.h"
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
namespace Hazel {
	static std::vector<char> readFile(const std::string& filename) {  // 参数改为std::filesystem::path
		std::ifstream file(filename, std::ios::ate | std::ios::binary);
		if (!file.is_open()) {
			throw std::runtime_error("failed to open file: " + filename);
		}
		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		return buffer;
	}
	VulkanShader::VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec)
	{
		m_Name = name;
		m_VertFilePath = vertFilePath;
		m_FragFilePath = fragFilePath;
		m_Spec = spec;
		m_PushConstantRanges = spec.pushConstantRanges;
		Reload();
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& computeFilePath, ShaderSpecification spec)
	{
		m_Name = name;
		m_Spec = spec;
		m_PushConstantRanges = spec.pushConstantRanges;
		m_ComputePath = computeFilePath;
		m_IsCompute = true;
		Reload();
	}

	void VulkanShader::Reload()
	{
		Renderer::Submit([instance = Ref(this)]() mutable {
			HZ_CORE_INFO("RT: Create Shader :{0}", instance->m_Name);
			instance->RT_Reload();
			});
	}

	void VulkanShader::RT_Reload()
	{
		// TODO: 先实现最简单的Shader载入功能
		if (m_IsCompute) {
			auto computCode = readFile(m_ComputePath);
			VkShaderModuleCreateInfo computCreateInfo{};
			computCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			computCreateInfo.codeSize = computCode.size();
			computCreateInfo.pCode = reinterpret_cast<const uint32_t*>(computCode.data());
			if (vkCreateShaderModule(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &computCreateInfo, nullptr, &m_ComputeShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
		}
		else {
			auto vertCode = readFile(m_VertFilePath);
			VkShaderModuleCreateInfo vertCreateInfo{};
			vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vertCreateInfo.codeSize = vertCode.size();
			vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
			if (vkCreateShaderModule(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &vertCreateInfo, nullptr, &m_VertShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
			auto fragCode = readFile(m_FragFilePath);
			VkShaderModuleCreateInfo fragCreateInfo{};
			fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragCreateInfo.codeSize = fragCode.size();
			fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
			if (vkCreateShaderModule(VulkanContext::GetCurrentDevice()->GetVulkanDevice(), &fragCreateInfo, nullptr, &m_FragShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
		}
		if (!m_Spec.bindings.empty()) {
			createDescriptorSetLayout();
			createDescriptorPool({ {0,100},{1,100} });
			createDescriptorSet();
		}
		else {
			HZ_CORE_WARN("RT: Shader Has No Descriptor :{0}", m_Name);
		}
	}

	void VulkanShader::createDescriptorSetLayout() {
		// 分组：set → bindings
		std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> setBindingsMap;

		for (const auto& binding : m_Spec.bindings) {
			VkDescriptorSetLayoutBinding layoutBinding{};
			layoutBinding.binding = binding.binding;
			layoutBinding.descriptorType = binding.type;
			layoutBinding.descriptorCount = binding.count;
			layoutBinding.stageFlags = binding.stageFlags;
			layoutBinding.pImmutableSamplers = nullptr;
			setBindingsMap[binding.set].push_back(layoutBinding);
		}

		if (setBindingsMap.empty()) {
			m_DescriptorSetLayouts.clear();
			return;
		}

		VkDevice device = Application::Get().GetRenderContext().As<VulkanContext>()->GetCurrentDevice()->GetVulkanDevice();

		// 步骤1：找到最大的setIndex，确定vector的大小
		uint32_t maxSetIndex = 0;
		for (const auto& [setIndex, _] : setBindingsMap) {
			if (setIndex > maxSetIndex) {
				maxSetIndex = setIndex;
			}
		}

		// 步骤2：初始化vector，大小为 maxSetIndex + 1（确保能容纳所有set）
		m_DescriptorSetLayouts.clear();
		m_DescriptorSetLayouts.resize(maxSetIndex + 1, VK_NULL_HANDLE);

		// 步骤3：按setIndex顺序创建布局，确保索引对应
		for (const auto& [setIndex, bindings] : setBindingsMap) {
			VkDescriptorSetLayoutCreateInfo layoutInfo{};
			layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayouts[setIndex]) != VK_SUCCESS) {
				throw std::runtime_error("Failed to create descriptor set layout!");
			}
		}
	}
	void VulkanShader::createDescriptorPool(const std::unordered_map<uint32_t, uint32_t>& setMaxCounts) {
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		std::unordered_set<uint32_t> uniqueSets;
		for (const auto& binding : m_Spec.bindings)
		{
			uniqueSets.insert(binding.set);
			uint32_t multiplier = setMaxCounts.at(binding.set);
			typeCountMap[binding.type] += binding.count * multiplier;
		}
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (const auto& kv : typeCountMap)
		{
			VkDescriptorPoolSize poolSize{};
			poolSize.type = kv.first;
			poolSize.descriptorCount = kv.second;
			poolSizes.push_back(poolSize);
		}
		uint32_t numSets = 0;
		for (uint32_t set : uniqueSets)
		{
			numSets += setMaxCounts.at(set);
		}
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data(); poolInfo.maxSets = numSets;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
	}

	void VulkanShader::createDescriptorSet()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		uint32_t maxSets = framesInFlight;
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		for (const auto& binding : m_Spec.bindings) {
			typeCountMap[binding.type] += binding.count * maxSets;
		}
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(typeCountMap.size());
		for (std::unordered_map<VkDescriptorType, uint32_t>::const_iterator it = typeCountMap.begin();
			it != typeCountMap.end();
			++it) {
			VkDescriptorPoolSize poolSize;
			poolSize.type = it->first;
			poolSize.descriptorCount = it->second;
			poolSizes.push_back(poolSize);
		}
		m_DescriptorSets.resize(maxSets);
		std::vector<VkDescriptorSetLayout> layouts(maxSets, m_DescriptorSetLayouts[0]);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = maxSets;
		allocInfo.pSetLayouts = layouts.data();

		VkResult allocateResult = vkAllocateDescriptorSets(
			device, &allocInfo, m_DescriptorSets.data());
		assert(allocateResult == VK_SUCCESS && "分配描述符集失败！");
	}

	std::vector<VkDescriptorSet> VulkanShader::createDescriptorSet(uint32_t targetSet)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		std::vector<VkDescriptorSet> descriptorSets(framesInFlight);

		if (targetSet >= m_DescriptorSetLayouts.size())
			return {};

		std::vector<VkDescriptorSetLayout> layouts(framesInFlight, m_DescriptorSetLayouts[targetSet]);

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = framesInFlight;
		allocInfo.pSetLayouts = layouts.data();

		VkResult result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
		assert(result == VK_SUCCESS && "分配描述符集失败");

		return descriptorSets;
	}

	void VulkanShader::Release() {
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		if (m_DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE;
		}
		for (auto m_DescriptorSetLayout : m_DescriptorSetLayouts) {
			if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
				vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
				m_DescriptorSetLayout = VK_NULL_HANDLE;
			}
		}
		if (m_FragShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, m_FragShaderModule, nullptr);
			m_FragShaderModule = VK_NULL_HANDLE;
		}
		if (m_VertShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, m_VertShaderModule, nullptr);
			m_VertShaderModule = VK_NULL_HANDLE;
		}
		m_Spec = {}; // 重置为默认状态（根据实际定义调整）
	}
	VulkanShader::~VulkanShader()
	{
		Release();
	}
}
