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
	VulkanShader::VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath,ShaderSpecification spec)
	{
		m_Name = name;
		m_VertFilePath = vertFilePath;
		m_FragFilePath = fragFilePath;
		m_Spec = spec;
		m_PushConstantRanges = spec.pushConstantRanges;
		Reload();
	}


	void VulkanShader::Reload()
	{
		Renderer::Submit([instance = Ref(this)]() mutable{
			HZ_CORE_INFO("Create Shader :{0}", instance->m_Name);
			instance->RT_Reload();
		});
	}

	
	void VulkanShader::RT_Reload()
	{
		// TODO: 先实现最简单的Shader载入功能
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
		createDescriptorSetLayout();
		createDescriptorPool(100);
		createDescriptorSet();
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
	void VulkanShader::createDescriptorPool(uint32_t maxMaterials)
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// Step1: 按 Set 聚合 binding 数量
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		for (const auto& binding : m_Spec.bindings)
		{
			uint32_t multiplier = 1;
			if (binding.set == 0)
				multiplier = Renderer::GetConfig().FramesInFlight;          // Set=0 每帧独立
			else if (binding.set == 1)
				multiplier = maxMaterials;            // Set=1 材质数量上限
			else
				multiplier = 1;                       // 其他 Set，可按需调整

			typeCountMap[binding.type] += binding.count * multiplier;
		}

		// Step2: 转换为 VkDescriptorPoolSize 数组
		std::vector<VkDescriptorPoolSize> poolSizes;
		poolSizes.reserve(typeCountMap.size());
		for (const auto& kv : typeCountMap)
		{
			VkDescriptorPoolSize poolSize{};
			poolSize.type = kv.first;
			poolSize.descriptorCount = kv.second;
			poolSizes.push_back(poolSize);
		}

		// Step3: 计算 maxSets
		uint32_t numSets = 0;
		std::unordered_set<uint32_t> uniqueSets;
		for (const auto& b : m_Spec.bindings)
			uniqueSets.insert(b.set);

		for (uint32_t set : uniqueSets)
		{
			if (set == 0)
				numSets += Renderer::GetConfig().FramesInFlight;
			else if (set == 1)
				numSets += maxMaterials;
			else
				numSets += 1;  // 其他 Set 按需
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = numSets;
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		VkResult createPoolResult = vkCreateDescriptorPool(device, &poolInfo, nullptr, &m_DescriptorPool);
		assert(createPoolResult == VK_SUCCESS && "创建 Shader 专用 DescriptorPool 失败！");
	}

	void VulkanShader::createDescriptorSet()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		// 获取并发帧数量
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		// 目前只使用Set=0，但为每个并发帧创建独立的描述符集
		uint32_t maxSets = framesInFlight;
		// 统计每种描述符类型的总需求（按并发帧数量计算）
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		for (const auto& binding : m_Spec.bindings) {
			// 每个绑定的总数量 = 单个集的数量 × 并发帧数量
			typeCountMap[binding.type] += binding.count * maxSets;
		}
		// 转换为VkDescriptorPoolSize数组
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
		// 为每个并发帧分配独立的描述符集（Set=0）
		m_DescriptorSets.resize(maxSets);  // 初始化向量容量
		std::vector<VkDescriptorSetLayout> layouts(maxSets, m_DescriptorSetLayouts[0]);  // 为Set=0提前创建好一个Set，用于提前绑定
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = maxSets;  // 一次分配所有并发帧的描述符集
		allocInfo.pSetLayouts = layouts.data();  // 所有集都使用Set=0的布局

		VkResult allocateResult = vkAllocateDescriptorSets(
			device, &allocInfo, m_DescriptorSets.data());
		assert(allocateResult == VK_SUCCESS && "分配描述符集失败！");
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
