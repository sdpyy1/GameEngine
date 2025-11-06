#include "hzpch.h"
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
#include "spirv_reflect.h"

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

	// helper key for merging set+binding
	struct SetBindingKey {
		uint32_t set;
		uint32_t binding;
		bool operator==(SetBindingKey const& o) const { return set == o.set && binding == o.binding; }
	};
	struct SetBindingKeyHash {
		std::size_t operator()(SetBindingKey const& k) const noexcept {
			// combine set and binding into single size_t
			return (static_cast<size_t>(k.set) << 32) ^ static_cast<size_t>(k.binding);
		}
	};

	// 反射单个 SPIR-V 模块并合并到 m_Spec（不会创建 VkShaderModule）
	void VulkanShader::ReflectSPIRVAndPopulateSpec(const std::vector<char>& spirvCode, VkShaderStageFlagBits stage)
	{
		if (spirvCode.empty())
			return;

		SpvReflectShaderModule reflModule;
		SpvReflectResult r = spvReflectCreateShaderModule(spirvCode.size(), spirvCode.data(), &reflModule);
		if (r != SPV_REFLECT_RESULT_SUCCESS) {
			HZ_CORE_ERROR("SPIRV-Reflect failed to create module: result={0}", static_cast<int>(r));
			return;
		}

		// 临时 map 用于合并相同 set+binding
		std::unordered_map<SetBindingKey, DescriptorBinding, SetBindingKeyHash> mergedBindings;
		// 先把当前 m_Spec 中已有的填入 mergedBindings（以便和新模块合并）
		for (const auto& existing : m_Spec.bindings) {
			SetBindingKey key{ existing.set, existing.binding };
			mergedBindings.emplace(key, existing);
		}

		// 枚举 descriptor bindings
		uint32_t bindingCount = 0;
		spvReflectEnumerateDescriptorBindings(&reflModule, &bindingCount, nullptr);
		std::vector<SpvReflectDescriptorBinding*> bindings;
		if (bindingCount > 0) {
			bindings.resize(bindingCount);
			spvReflectEnumerateDescriptorBindings(&reflModule, &bindingCount, bindings.data());
		}

		for (uint32_t i = 0; i < bindingCount; ++i) {
			const SpvReflectDescriptorBinding* b = bindings[i];
			if (!b) continue;

			SetBindingKey key{ b->set, b->binding };
			auto it = mergedBindings.find(key);
			if (it == mergedBindings.end()) {
				DescriptorBinding desc{};
				desc.binding = b->binding;
				desc.set = b->set;
				desc.type = static_cast<VkDescriptorType>(b->descriptor_type);
				desc.count = b->count;
				desc.stageFlags = stage;
				mergedBindings.emplace(key, desc);
			}
			else {
				// 合并 stageFlags（OR）
				it->second.stageFlags = static_cast<VkShaderStageFlags>(it->second.stageFlags | stage);
				// 如果类型或 count 不一致，记录警告并保留第一次发现的类型/count
				if (it->second.type != static_cast<VkDescriptorType>(b->descriptor_type)) {
					HZ_CORE_WARN("Reflection: descriptor type mismatch at set={}, binding={} (kept first).", b->set, b->binding);
				}
				if (it->second.count != b->count) {
					HZ_CORE_WARN("Reflection: descriptor count mismatch at set={}, binding={} (kept first).", b->set, b->binding);
				}
			}

			// 打印信息便于调试
			HZ_CORE_INFO("Reflect: stage={0}, set={1}, binding={2}, name={3}, type={4}, count={5}",
				(uint32_t)stage, b->set, b->binding, (b->name ? b->name : "null"), b->descriptor_type, b->count);
		}

		// 处理 push constants：合并相同 (offset,size) 的范围并 OR stageFlags
		uint32_t pcCount = 0;
		spvReflectEnumeratePushConstantBlocks(&reflModule, &pcCount, nullptr);
		std::vector<SpvReflectBlockVariable*> pcs;
		if (pcCount > 0) {
			pcs.resize(pcCount);
			spvReflectEnumeratePushConstantBlocks(&reflModule, &pcCount, pcs.data());
		}

		// 先把已有 push constants 存到临时 vector，便于合并
		std::vector<PushConstantRange> mergedPCs = m_Spec.pushConstantRanges;

		for (uint32_t i = 0; i < pcCount; ++i) {
			const SpvReflectBlockVariable* p = pcs[i];
			if (!p) continue;

			bool mergedExisting = false;
			for (auto& existing : mergedPCs) {
				if (existing.offset == p->offset && existing.size == p->size) {
					existing.shaderStage = static_cast<VkShaderStageFlags>(existing.shaderStage | stage);
					mergedExisting = true;
					break;
				}
			}
			if (!mergedExisting) {
				PushConstantRange pc{};
				pc.shaderStage = stage;
				pc.offset = p->offset;
				pc.size = p->size;
				mergedPCs.push_back(pc);
				HZ_CORE_INFO("Reflect PushConstant: offset={0}, size={1}, stage={2}", pc.offset, pc.size, (uint32_t)stage);
			}
		}

		// 把 mergedBindings 的结果写回 m_Spec.bindings，保持稳定顺序（按 set then binding 排序）
		m_Spec.bindings.clear();
		// collect keys and sort for deterministic order
		std::vector<std::pair<SetBindingKey, DescriptorBinding>> tmp;
		tmp.reserve(mergedBindings.size());
		for (auto& kv : mergedBindings) tmp.push_back(kv);
		std::sort(tmp.begin(), tmp.end(), [](auto const& a, auto const& b) {
			if (a.first.set != b.first.set) return a.first.set < b.first.set;
			return a.first.binding < b.first.binding;
			});
		for (auto& p : tmp) m_Spec.bindings.push_back(p.second);

		// 更新 push constants
		m_Spec.pushConstantRanges = std::move(mergedPCs);

		spvReflectDestroyShaderModule(&reflModule);
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath)
	{
		m_Name = name;
		m_VertFilePath = vertFilePath;
		m_FragFilePath = fragFilePath;
		Reload();
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& computeFilePath)
	{
		m_Name = name;
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
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();

		// 先读取 SPIR-V 字节码并反射填充 m_Spec（反射与 VkShaderModule 创建解耦）
		if (m_IsCompute) {
			auto compCode = readFile(m_ComputePath);
			ReflectSPIRVAndPopulateSpec(compCode, VK_SHADER_STAGE_COMPUTE_BIT);
			// 创建 compute shader module（保留你原来行为）
			VkShaderModuleCreateInfo computCreateInfo{};
			computCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			computCreateInfo.codeSize = compCode.size();
			computCreateInfo.pCode = reinterpret_cast<const uint32_t*>(compCode.data());
			if (vkCreateShaderModule(device, &computCreateInfo, nullptr, &m_ComputeShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
		}
		else {
			auto vertCode = readFile(m_VertFilePath);
			auto fragCode = readFile(m_FragFilePath);

			// 只做反射来填充 m_Spec（分别对 vert 和 frag）
			ReflectSPIRVAndPopulateSpec(vertCode, VK_SHADER_STAGE_VERTEX_BIT);
			ReflectSPIRVAndPopulateSpec(fragCode, VK_SHADER_STAGE_FRAGMENT_BIT);

			// 再创建 shader module（保留原逻辑）
			VkShaderModuleCreateInfo vertCreateInfo{};
			vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vertCreateInfo.codeSize = vertCode.size();
			vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
			if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &m_VertShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
			VkShaderModuleCreateInfo fragCreateInfo{};
			fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			fragCreateInfo.codeSize = fragCode.size();
			fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
			if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &m_FragShaderModule) != VK_SUCCESS) {
				throw std::runtime_error("failed to create shader module!");
			}
		}

		// 使用 m_Spec 创建描述符布局/池/集（维持你原有行为）
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
