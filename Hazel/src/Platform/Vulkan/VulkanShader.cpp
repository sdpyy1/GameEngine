#include "hzpch.h"
#include "VulkanShader.h"
#include "Hazel/Renderer/Renderer.h"
#include "VulkanContext.h"
namespace Hazel {
	static std::vector<char> readFile(const std::string& filename) {  // ������Ϊstd::filesystem::path
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
		Reload();
	}

	VulkanShader::VulkanShader(const std::string& name, const std::string& vertFilePath, const std::string& fragFilePath, ShaderSpecification spec, VkDevice device)
	{
		m_Name = name;
		m_VertFilePath = vertFilePath;
		m_FragFilePath = fragFilePath;
		m_Spec = spec;

		auto vertCode = readFile(m_VertFilePath);
		VkShaderModuleCreateInfo vertCreateInfo{};
		vertCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		vertCreateInfo.codeSize = vertCode.size();
		vertCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
		if (vkCreateShaderModule(device, &vertCreateInfo, nullptr, &m_VertShaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		auto fragCode = readFile(m_FragFilePath);
		VkShaderModuleCreateInfo fragCreateInfo{};
		fragCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		fragCreateInfo.codeSize = fragCode.size();
		fragCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
		if (vkCreateShaderModule(device, &fragCreateInfo, nullptr, &m_FragShaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		layoutBindings.reserve(m_Spec.bindings.size());
		for (const auto& binding : m_Spec.bindings) {
			VkDescriptorSetLayoutBinding layoutBinding;
			layoutBinding.binding = binding.binding;
			layoutBinding.descriptorType = binding.type;
			layoutBinding.descriptorCount = binding.count;
			layoutBinding.stageFlags = binding.stageFlags;
			layoutBinding.pImmutableSamplers = nullptr;  // �ǹ̶�����������Ϊnullptr
			layoutBindings.push_back(layoutBinding);
		}

		// ��������������
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
		// ��ȡ����֡��������˫����Ϊ2��������Ϊ3��
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		// Ŀǰֻʹ��Set=0����Ϊÿ������֡������������������
		uint32_t maxSets = framesInFlight;

		// ͳ��ÿ�����������͵������󣨰�����֡�������㣩
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		for (const auto& binding : m_Spec.bindings) {
			// ÿ���󶨵������� = ������������ �� ����֡����
			typeCountMap[binding.type] += binding.count * maxSets;
		}

		// ת��ΪVkDescriptorPoolSize����
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

		// �����������أ��������������в���֡������������
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets;  // ���пɷ���������������� = ����֡����

		VkResult createPoolResult = vkCreateDescriptorPool(
			device, &poolInfo, nullptr, &m_DescriptorPool);
		assert(createPoolResult == VK_SUCCESS && "������������ʧ�ܣ�");

		// Ϊÿ������֡�������������������Set=0��
		m_DescriptorSets.resize(maxSets);  // ��ʼ����������
		std::vector<VkDescriptorSetLayout> layouts(maxSets, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = maxSets;  // һ�η������в���֡����������
		allocInfo.pSetLayouts = layouts.data();  // ���м���ʹ��Set=0�Ĳ���

		VkResult allocateResult = vkAllocateDescriptorSets(
			device, &allocInfo, m_DescriptorSets.data());
		assert(allocateResult == VK_SUCCESS && "������������ʧ�ܣ�");


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
		// TODO: ��ʵ����򵥵�Shader���빦��
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
		// ��ǰ���ÿ��Shader��Ҳ���Ǹ�ÿ��Pass����һ��Pool��
		createDescriptorSet();
	}

	void VulkanShader::createDescriptorSetLayout() {
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		layoutBindings.reserve(m_Spec.bindings.size());
		for (const auto& binding : m_Spec.bindings) {
			VkDescriptorSetLayoutBinding layoutBinding;
			layoutBinding.binding = binding.binding;
			layoutBinding.descriptorType = binding.type;
			layoutBinding.descriptorCount = binding.count;
			layoutBinding.stageFlags = binding.stageFlags;
			layoutBinding.pImmutableSamplers = nullptr;  // �ǹ̶�����������Ϊnullptr
			layoutBindings.push_back(layoutBinding);
		}

		// ��������������
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		if (vkCreateDescriptorSetLayout(Application::Get().GetRenderContext().As<VulkanContext>()->GetCurrentDevice()->GetVulkanDevice(), &layoutInfo, nullptr, &m_DescriptorSetLayout) != VK_SUCCESS) {
			throw std::runtime_error("Failed to create descriptor set layout!");
		}
	}

	void VulkanShader::createDescriptorSet()
	{
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		// ��ȡ����֡����
		uint32_t framesInFlight = Renderer::GetConfig().FramesInFlight;
		// Ŀǰֻʹ��Set=0����Ϊÿ������֡������������������
		uint32_t maxSets = framesInFlight;
		// ͳ��ÿ�����������͵������󣨰�����֡�������㣩
		std::unordered_map<VkDescriptorType, uint32_t> typeCountMap;
		for (const auto& binding : m_Spec.bindings) {
			// ÿ���󶨵������� = ������������ �� ����֡����
			typeCountMap[binding.type] += binding.count * maxSets;
		}
		// ת��ΪVkDescriptorPoolSize����
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
		// �����������أ��������������в���֡������������
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = maxSets * 101;  // ���пɷ���������������� = ����֡����  + ����֧��100��Set
		VkResult createPoolResult = vkCreateDescriptorPool(
			device, &poolInfo, nullptr, &m_DescriptorPool);
		assert(createPoolResult == VK_SUCCESS && "������������ʧ�ܣ�");
		// Ϊÿ������֡�������������������Set=0��
		m_DescriptorSets.resize(maxSets);  // ��ʼ����������
		std::vector<VkDescriptorSetLayout> layouts(maxSets, m_DescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = maxSets;  // һ�η������в���֡����������
		allocInfo.pSetLayouts = layouts.data();  // ���м���ʹ��Set=0�Ĳ���

		VkResult allocateResult = vkAllocateDescriptorSets(
			device, &allocInfo, m_DescriptorSets.data());
		assert(allocateResult == VK_SUCCESS && "������������ʧ�ܣ�");
	}
	void VulkanShader::Release() {
		VkDevice device = VulkanContext::GetCurrentDevice()->GetVulkanDevice();
		if (m_DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(device, m_DescriptorPool, nullptr);
			m_DescriptorPool = VK_NULL_HANDLE; 
		}
		if (m_DescriptorSetLayout != VK_NULL_HANDLE) {
			vkDestroyDescriptorSetLayout(device, m_DescriptorSetLayout, nullptr);
			m_DescriptorSetLayout = VK_NULL_HANDLE;
		}

		if (m_FragShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, m_FragShaderModule, nullptr);
			m_FragShaderModule = VK_NULL_HANDLE;
		}
		if (m_VertShaderModule != VK_NULL_HANDLE) {
			vkDestroyShaderModule(device, m_VertShaderModule, nullptr);
			m_VertShaderModule = VK_NULL_HANDLE;
		}
		m_Spec = {}; // ����ΪĬ��״̬������ʵ�ʶ��������
	}
	VulkanShader::~VulkanShader()
	{
		Release();
	}
}
