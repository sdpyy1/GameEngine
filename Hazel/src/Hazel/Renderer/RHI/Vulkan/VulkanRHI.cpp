#include "hzpch.h"
#include "VulkanRHI.h"
#include "VulkanUtil.h"
#include "VulkanRHIResource.h"
#define VMA_IMPLEMENTATION

#define VULKAN_VERSION VK_API_VERSION_1_2

namespace GameEngine
{
	VulkanDynamicRHI::VulkanDynamicRHI(const RHIConfig& config): DynamicRHI(config)
	{
		CreateInstance();
		CreatePhysicalDevice();
        CreateLogicalDevice();
        CreateQueues();
        CreateMemoryAllocator();
        CreateDescriptorPool();
	}

	void VulkanDynamicRHI::CreateInstance()
	{
		// volk
        if (volkInitialize() != VK_SUCCESS) {
            LOG_ERROR("Volk initialize failed!");
            return;
        }
		// 获取可用的实例层
		{
			uint32_t layerCount;
			vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
			m_AvailableLayers = std::vector<VkLayerProperties>(layerCount);
			vkEnumerateInstanceLayerProperties(&layerCount, m_AvailableLayers.data());
			/*for (auto& layer : availableLayers)
			{
				LOG_TRACE("Layer: {}, Description: {}, Spec Version: {}, Implementation Version: {}",layer.layerName,layer.description,layer.specVersion,layer.implementationVersion);
			}*/
		}

		// VkApplicationInfo
		VkApplicationInfo appInfo = {};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Toy Render Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VULKAN_VERSION;


		// VkInstance
		VkInstanceCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto validationLayers = VulkanUtil::GetDebugMessengerCreateInfo();
		auto validationFeature = VulkanUtil::GetValidationFeatureCreateInfo();
		std::vector<const char*> layers;
		if (m_Config.debug)
		{
			for (auto& layer : INSTANCE_LAYERS) layers.push_back(layer);

			createInfo.enabledLayerCount = (uint32_t)layers.size();
			createInfo.ppEnabledLayerNames = layers.data();
			createInfo.pNext = &validationLayers;
			validationLayers.pNext = &validationFeature;
		}

		auto instanceExtentions = VulkanUtil::GetRequiredInstanceExtensions(m_Config.debug);
		createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtentions.size());
		createInfo.ppEnabledExtensionNames = instanceExtentions.data();

		// 创建Vulkan实例
        VK_CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &m_Instance));
		volkLoadInstance(m_Instance);
		
		//创建DebugMessager
		if (m_Config.debug)
		{
			VkDebugUtilsMessengerCreateInfoEXT info = VulkanUtil::GetDebugMessengerCreateInfo();
			if (vkCreateDebugUtilsMessengerEXT(m_Instance, &info, nullptr, &m_DebugMessenger) != VK_SUCCESS){LOG_ERROR("Failed to set up debug messenger!");}
		}
	}

	void VulkanDynamicRHI::CreatePhysicalDevice()
	{
        // 获取可用物理设备
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (auto& device : devices) {
            vkGetPhysicalDeviceProperties(device, &m_PhysicalDeviceProperties);
            // 直接找AMD或NVIDIA，找到就创建，不支持集成GPU
            for (auto target : TARGET_DEVICES)
            {
                std::string name = std::string(m_PhysicalDeviceProperties.deviceName);
                std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
                if (name.find(target) != std::string::npos)
                {
                    {
                        LOG_TRACE_TAG("PhysicalDevice", LOG_LINE);
                        LOG_TRACE_TAG("PhysicalDevice", "Device Name: {}", m_PhysicalDeviceProperties.deviceName);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferColorSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferColorSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferDepthSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferDepthSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "framebufferStencilSampleCounts: {}", m_PhysicalDeviceProperties.limits.framebufferStencilSampleCounts);
                        LOG_TRACE_TAG("PhysicalDevice", "maxColorAttachments: {}", m_PhysicalDeviceProperties.limits.maxColorAttachments);
                        LOG_TRACE_TAG("PhysicalDevice", "maxDescriptorSetInputAttachments: {}", m_PhysicalDeviceProperties.limits.maxDescriptorSetInputAttachments);
                        LOG_TRACE_TAG("PhysicalDevice", "maxDescriptorSetUniformBuffers: {}", m_PhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffers);
                        LOG_TRACE_TAG("PhysicalDevice", "maxFramebufferLayers: {}", m_PhysicalDeviceProperties.limits.maxFramebufferLayers);
                        LOG_TRACE_TAG("PhysicalDevice", "maxPushConstantsSize: {} bytes", m_PhysicalDeviceProperties.limits.maxPushConstantsSize);
                        LOG_TRACE_TAG("PhysicalDevice", "maxBoundDescriptorSets: {}", m_PhysicalDeviceProperties.limits.maxBoundDescriptorSets);
                        LOG_TRACE_TAG("PhysicalDevice", "maxComputeWorkGroupInvocations: {}", m_PhysicalDeviceProperties.limits.maxComputeWorkGroupInvocations);
                        LOG_TRACE_TAG("PhysicalDevice", "minStorageBufferOffsetAlignment: {} bytes", m_PhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);
                        LOG_TRACE_TAG("PhysicalDevice", LOG_LINE);
                    }

                    //初始化存储物理设备支持的各种属性，特性，扩展，内存特性，队列信息
                    //vkGetPhysicalDeviceFeatures2(device, &fetures2);
                    vkGetPhysicalDeviceFeatures(device, &m_PhysicalDeviceFeatures);
                    vkGetPhysicalDeviceMemoryProperties(device, &m_PhysicalDeviceMemoryProperties);
    
                    // 获取所有支持的拓展
                    uint32_t extCount = 0;
                    vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, nullptr);
                    std::vector<VkExtensionProperties> extensions(extCount);
                    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
                    {
                        for (auto& ext : extensions)
                        {
                            m_PhysicalDeviceSupportedExtensions.push_back(ext.extensionName);
                            // LOG_TRACE_TAG("PhysicalDevice", "Support Extension: {}", ext.extensionName);
                        }
                    }

                    if (m_Config.enableRayTracing)
                    {
                        //获取光追管线特性
                        m_PhysicalDeviceRayTracingPipelineProperties = {};
                        m_PhysicalDeviceRayTracingPipelineProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;

                        VkPhysicalDeviceProperties2 deviceProperties2 = {};
                        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
                        deviceProperties2.pNext = &m_PhysicalDeviceRayTracingPipelineProperties;
                        vkGetPhysicalDeviceProperties2(device, &deviceProperties2);
                    }

                    m_PhysicalDevice = device;
                    return;
                }
            }
        }
        LOG_ERROR("Target device not found!");
    }

    void VulkanDynamicRHI::CreateLogicalDevice() {
        // 获取队列族信息
        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);
        m_QueueFamilyProperties.resize(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, m_QueueFamilyProperties.data());

        for (auto& index : m_QueueIndices) index = -1;


        /*for (auto& queueFamily : m_QueueFamilyProperties)
        {
            LOG_TRACE_TAG("Queue", "Queue Count: {}", queueFamily.queueCount);
            LOG_TRACE_TAG("Queue", "Queue Flags: {}", VulkanUtil::QueueFlagsToString(queueFamily.queueFlags));
        }*/

        // 遍历队列族，寻找需要的队列的支持，找到支持的队列组后后续族就不需要创建了，每个类型队列会创建2个Queue
        std::vector<uint32_t> allocatedCounts(m_QueueFamilyProperties.size()); // 记录每个队列组需要申请的Queue数量，提供给逻辑设备创建
        for (int i = 0; i < m_QueueFamilyProperties.size(); i++)
        {
            auto& queueFamily = m_QueueFamilyProperties[i];
            uint32_t queueCount = queueFamily.queueCount;
            if (queueCount > MAX_QUEUE_CNT && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && m_QueueIndices[QUEUE_TYPE_GRAPHICS] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_GRAPHICS] = i;
                queueCount -= MAX_QUEUE_CNT;
            }
            if (queueCount > MAX_QUEUE_CNT &&queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT &&m_QueueIndices[QUEUE_TYPE_COMPUTE] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_COMPUTE] = i;
                queueCount -= MAX_QUEUE_CNT;
            }
            if (queueCount > MAX_QUEUE_CNT &&queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&m_QueueIndices[QUEUE_TYPE_TRANSFER] < 0)
            {
                m_QueueIndices[QUEUE_TYPE_TRANSFER] = i;
                queueCount -= MAX_QUEUE_CNT;
            }

            allocatedCounts[i] = queueFamily.queueCount - queueCount;

            for (int i = 0; i < QUEUE_TYPE_MAX_ENUM; i++) if (m_QueueIndices[i] < 0) LOG_ERROR("Fail to allocate queue!");

        }

        // 创建设备请求信息
        VkDeviceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        //队列信息 要声明每个队列组需要创建的Queue数量
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        for (int i = 0; i < allocatedCounts.size(); i++)
        {
            if (allocatedCounts[i] == 0) continue;
            std::vector<float> priorities(allocatedCounts[i], 1.0f);

            VkDeviceQueueCreateInfo queueCreateInfo = {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = i;                               //队列族
            queueCreateInfo.queueCount = allocatedCounts[i];                    //队列数目，多个队列可以支持并行异步计算
            queueCreateInfo.pQueuePriorities = QUEUE_PRIORITIES;                //优先级
            queueCreateInfos.push_back(queueCreateInfo);
        }
        createInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
        createInfo.pQueueCreateInfos = queueCreateInfos.data();

        //扩展信息
        std::vector<const char*> deviceExtentions;
        for (auto extention : DEVICE_EXTENTIONS) { 
            if (!VulkanUtil::IsExtensionSupported(m_PhysicalDeviceSupportedExtensions, extention)) continue;
            deviceExtentions.push_back(extention);
        };
        if (m_Config.enableRayTracing) { for (auto extention : RAY_TRACING_DEVICE_EXTENTIONS) deviceExtentions.push_back(extention); }
        createInfo.enabledExtensionCount = (uint32_t)deviceExtentions.size();
        createInfo.ppEnabledExtensionNames = deviceExtentions.data();

        // 设备Layer已被废弃
        /*std::vector<const char*> devicelayers;
        if (m_Config.debug)
        {
            for (auto layer : DEVICE_LAYERS) devicelayers.push_back(layer);

            createInfo.enabledLayerCount = (uint32_t)devicelayers.size();
            createInfo.ppEnabledLayerNames = devicelayers.data();
        }*/

        // 特性支持
        VkPhysicalDeviceFeatures deviceFeatures = {};       //设备特性信息
        deviceFeatures.samplerAnisotropy = VK_TRUE;         //请求各向异性采样支持
        deviceFeatures.geometryShader = VK_TRUE;            //几何着色器支持
        deviceFeatures.tessellationShader = VK_TRUE;        //曲面细分着色器支持
        deviceFeatures.pipelineStatisticsQuery = VK_TRUE;   //管线统计查询支持
        deviceFeatures.fillModeNonSolid = VK_TRUE;          //线框模式
        deviceFeatures.multiDrawIndirect = VK_TRUE;         //多重间接绘制
        deviceFeatures.independentBlend = VK_TRUE;          //MRT单独设置每个混合状态
        deviceFeatures.drawIndirectFirstInstance = VK_TRUE; //允许间接绘制的firstInstance不为0
        deviceFeatures.shaderInt64 = VK_TRUE;               //64位支持
        deviceFeatures.shaderFloat64 = VK_TRUE;
        createInfo.pEnabledFeatures = &deviceFeatures;

        VkPhysicalDeviceVulkan12Features vulkan12Features{};                                        //1.2版本的其他支持
        vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
        vulkan12Features.samplerFilterMinmax = VK_TRUE;                                             //采样器的过滤模式，用于Hiz
        //bindless支持，描述符索引
        vulkan12Features.runtimeDescriptorArray = VK_TRUE;                                          //SPIR-V 中使用动态数组
        vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;                        //DescriptorSet Layout 的 Binding 中使用可变大小的 AoD
        vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;                       //SPIR-V 中通过 nonuniformEXT Decoration 用于非 Uniform 变量下标索引资源数组
        vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
        vulkan12Features.descriptorIndexing = VK_TRUE;
        vulkan12Features.bufferDeviceAddress = VK_TRUE;
        createInfo.pNext = &vulkan12Features;

        VkPhysicalDeviceVertexInputDynamicStateFeaturesEXT dynamicVertexInputFeatures = {};         //动态顶点输入描述
        dynamicVertexInputFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VERTEX_INPUT_DYNAMIC_STATE_FEATURES_EXT;
        dynamicVertexInputFeatures.vertexInputDynamicState = VK_TRUE;
        vulkan12Features.pNext = &dynamicVertexInputFeatures;

        if (m_Config.enableRayTracing)
        {
            VkPhysicalDeviceBufferDeviceAddressFeaturesEXT deviceAddressfeture = {};                //请求内存地址信息
            deviceAddressfeture.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT;
            deviceAddressfeture.bufferDeviceAddress = VK_TRUE;

            VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};    //rt加速结构
            accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
            accelerationStructureFeatures.accelerationStructure = true;

            VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};          //rt管线
            rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
            rayTracingPipelineFeatures.rayTracingPipeline = true;

            VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};                              //rt查询
            rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
            rayQueryFeatures.rayQuery = VK_TRUE;
            dynamicVertexInputFeatures.pNext = &deviceAddressfeture;
            deviceAddressfeture.pNext = &accelerationStructureFeatures;
            accelerationStructureFeatures.pNext = &rayTracingPipelineFeatures;
            rayTracingPipelineFeatures.pNext = &rayQueryFeatures;
        }
        VK_CHECK_RESULT(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice));
        volkLoadDevice(m_LogicalDevice);  //volk
    }

    void VulkanDynamicRHI::CreateQueues()
    {
        std::vector<uint32_t> offsets(m_QueueFamilyProperties.size(), { 0 });
        for (uint32_t i = 0; i < QUEUE_TYPE_MAX_ENUM; i++)
        {
            for (uint32_t j = 0; j < MAX_QUEUE_CNT; j++)
            {
                VkQueue queue;
                vkGetDeviceQueue(m_LogicalDevice, m_QueueIndices[i], offsets[i], &queue);

                RHIQueueInfo info =
                {
                    (QueueType)i,
                    j
                };
                m_Queues[i][j] = std::make_shared<VulkanRHIQueue>(info, queue, m_QueueIndices[i]);
                RegisterResource(m_Queues[i][j]);

                offsets[i]++;
            }
        }
    }

	void VulkanDynamicRHI::CreateMemoryAllocator()
	{
        // volk集成vma: https://zhuanlan.zhihu.com/p/634912614 
        // vma官方文档: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/index.html
        // 对照该表，不同vulkan版本有不同的绑定要求: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/struct_vma_vulkan_functions.html

        VmaVulkanFunctions vulkanFunctions{};
        vulkanFunctions.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
        vulkanFunctions.vkAllocateMemory = vkAllocateMemory;
        vulkanFunctions.vkFreeMemory = vkFreeMemory;
        vulkanFunctions.vkMapMemory = vkMapMemory;
        vulkanFunctions.vkUnmapMemory = vkUnmapMemory;
        vulkanFunctions.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
        vulkanFunctions.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
        vulkanFunctions.vkBindBufferMemory = vkBindBufferMemory;
        vulkanFunctions.vkBindImageMemory = vkBindImageMemory;
        vulkanFunctions.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
        vulkanFunctions.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
        vulkanFunctions.vkCreateBuffer = vkCreateBuffer;
        vulkanFunctions.vkDestroyBuffer = vkDestroyBuffer;
        vulkanFunctions.vkCreateImage = vkCreateImage;
        vulkanFunctions.vkDestroyImage = vkDestroyImage;
        vulkanFunctions.vkCmdCopyBuffer = vkCmdCopyBuffer;
        // vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2KHR;
        // vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2KHR;
        // vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2KHR;
        // vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2KHR;
        // vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2KHR;
        vulkanFunctions.vkGetBufferMemoryRequirements2KHR = vkGetBufferMemoryRequirements2;
        vulkanFunctions.vkGetImageMemoryRequirements2KHR = vkGetImageMemoryRequirements2;
        vulkanFunctions.vkBindBufferMemory2KHR = vkBindBufferMemory2;
        vulkanFunctions.vkBindImageMemory2KHR = vkBindImageMemory2;
        vulkanFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vkGetPhysicalDeviceMemoryProperties2;
        // vulkanFunctions.vkGetDeviceBufferMemoryRequirements = vkGetDeviceBufferMemoryRequirements;
        // vulkanFunctions.vkGetDeviceImageMemoryRequirements = vkGetDeviceImageMemoryRequirements;

        VmaAllocatorCreateInfo allocatorCreateInfo = {};
        allocatorCreateInfo.vulkanApiVersion = VULKAN_VERSION;
        allocatorCreateInfo.physicalDevice = m_PhysicalDevice;
        allocatorCreateInfo.device = m_LogicalDevice;
        allocatorCreateInfo.instance = m_Instance;
        allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;
        allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT; // 加上这个才能获取地址

        VK_CHECK_RESULT(vmaCreateAllocator(&allocatorCreateInfo, &m_MemoryAllocator));
	}

	void VulkanDynamicRHI::CreateDescriptorPool()
	{
        std::vector<VkDescriptorPoolSize> descriptorPoolSizes = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 4096 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4096 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4096 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 4096 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 4096 },
        };

        //描述符池信息
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
        poolInfo.pPoolSizes = descriptorPoolSizes.data();
        poolInfo.maxSets = 8192;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;   // 使得描述符可以实时更新

        VK_CHECK_RESULT(vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool));
	}


    void VulkanDynamicRHI::CreateImmediateCommand()
    {
        m_ImmediateCommandContext = std::make_shared<VulkanRHICommandContextImmediate>();
        RegisterResource(m_ImmediateCommandContext);

        CommandListImmediateInfo info;
        info.context = m_ImmediateCommandContext;
        m_ImmediateCommandList = std::make_shared<RHICommandListImmediate>(info);
    }

    RHIQueueRef VulkanDynamicRHI::GetQueue(const RHIQueueInfo& info)
    {
        return m_Queues[info.type][info.index];

    }

	RHISurfaceRef VulkanDynamicRHI::CreateSurface(GLFWwindow* window)
	{
        RHISurfaceRef surface = std::make_shared<VulkanRHISurface>(window);
        RegisterResource(surface);

        return surface;
	}

	RHISwapchainRef VulkanDynamicRHI::CreateSwapChain(const RHISwapchainInfo& info)
	{
         RHISwapchainRef swapchain = std::make_shared<VulkanRHISwapchain>(info);
         RegisterResource(swapchain);

        return swapchain;
	}

	RHICommandPoolRef VulkanDynamicRHI::CreateCommandPool(const RHICommandPoolInfo& info)
	{
        RHICommandPoolRef commandPool = std::make_shared<VulkanRHICommandPool>(info);
        RegisterResource(commandPool);

        return commandPool;
	}

	RHICommandContextRef VulkanDynamicRHI::CreateCommandContext(RHICommandPoolRef pool)
	{
        RHICommandContextRef commandContext = std::make_shared<VulkanRHICommandContext>(pool);
        RegisterResource(commandContext);

        return commandContext;
	}

    RHIFenceRef VulkanDynamicRHI::CreateFence(bool signaled)
    {
        RHIFenceRef fence = std::make_shared<VulkanRHIFence>(signaled);
        RegisterResource(fence);

        return fence;
    }

    RHISemaphoreRef VulkanDynamicRHI::CreateSemaphore()
    {
        RHISemaphoreRef semaphore = std::make_shared<VulkanRHISemaphore>();
        RegisterResource(semaphore);

        return semaphore;
    }

	RHITextureRef VulkanDynamicRHI::CreateTexture(const RHITextureInfo& info)
	{
        RHITextureRef texture = std::make_shared<VulkanRHITexture>(info);
        RegisterResource(texture);

        return texture;
	}

	RHISamplerRef VulkanDynamicRHI::CreateSampler(const RHISamplerInfo& info)
	{
        RHISamplerRef sampler = std::make_shared<VulkanRHISampler>(info);
        RegisterResource(sampler);

        return sampler;
	}

	RHIShaderRef VulkanDynamicRHI::CreateShader(const RHIShaderInfo& info)
	{
        RHIShaderRef shader = std::make_shared<VulkanRHIShader>(info);
        RegisterResource(shader);

        return shader;
	}

	RHICommandListImmediateRef VulkanDynamicRHI::GetImmediateCommandList(bool start)
	{
        if (!m_ImmediateCommandList) {
            CreateImmediateCommand();
        }
        if (start) {
            CAST<VulkanRHICommandContextImmediate>(m_ImmediateCommandContext)->BeginSingleTimeCommand();
        }
        return m_ImmediateCommandList;

	}

	RHITextureViewRef VulkanDynamicRHI::CreateTextureView(const RHITextureViewInfo& info)
	{
        RHITextureViewRef textureView = std::make_shared<VulkanRHITextureView>(info);
        RegisterResource(textureView);

        return textureView;
	}

	RHIBufferRef VulkanDynamicRHI::CreateBuffer(const RHIBufferInfo& info)
	{
        RHIBufferRef buffer = std::make_shared<VulkanRHIBuffer>(info);
        RegisterResource(buffer);

        return buffer;
	}

	RHIRootSignatureRef VulkanDynamicRHI::CreateRootSignature(const RHIRootSignatureInfo& info)
	{
        RHIRootSignatureRef rootSignature = std::make_shared<VulkanRHIRootSignature>(info);
        RegisterResource(rootSignature);

        return rootSignature;
	}

	RHIGraphicsPipelineRef VulkanDynamicRHI::CreateGraphicsPipeline(const RHIGraphicsPipelineInfo& info)
	{
        RHIGraphicsPipelineRef graphicsPipeline = std::make_shared<VulkanRHIGraphicsPipeline>(info);
        RegisterResource(graphicsPipeline);

        return graphicsPipeline;
	}

	RHIRenderPassRef VulkanDynamicRHI::CreateRenderPass(const RHIRenderPassInfo& info)
	{
        RHIRenderPassRef renderPass = std::make_shared<VulkanRHIRenderPass>(info);
        RegisterResource(renderPass);

        return renderPass;
	}

	VkRenderPass VulkanDynamicRHI::CreateVkRenderPass(const VulkanUtil::VulkanRenderPassAttachments& info)
	{
        bool hasDepth = (info.depthStencilAttachment.format != VK_FORMAT_UNDEFINED);
        std::vector<VkAttachmentDescription> attachments;
        std::vector<VkAttachmentReference> colorReferences;
        VkAttachmentReference depthReference = {};

        for (const VkAttachmentDescription& attachment : info.colorAttachments)  attachments.push_back(attachment);
        for (VkAttachmentDescription& attachment : attachments)
        {
            attachment.stencilLoadOp = attachment.loadOp;   // 写死
            attachment.stencilStoreOp = attachment.storeOp;
            attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }
        if (hasDepth)
        {
            VkAttachmentDescription attachment = info.depthStencilAttachment;
            attachment.stencilLoadOp = attachment.loadOp;   // 写死
            attachment.stencilStoreOp = attachment.storeOp;
            attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            attachments.push_back(attachment);
        }

        for (uint32_t i = 0; i < attachments.size(); i++) colorReferences.push_back({ i, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
        VkAttachmentReference depthAttachmentReference = { (uint32_t)attachments.size() - 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

        // 不支持subpass了 艹
        VkSubpassDescription subpass = {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = (uint32_t)colorReferences.size();
        subpass.pColorAttachments = colorReferences.data();
        subpass.pDepthStencilAttachment = hasDepth ? &depthAttachmentReference : nullptr;
        subpass.inputAttachmentCount = 0;
        subpass.pInputAttachments = nullptr;
        subpass.preserveAttachmentCount = 0;
        subpass.pPreserveAttachments = nullptr;
        subpass.pResolveAttachments = nullptr;

        // 屏障在外面显式的添加 不在pass里加了？
        // std::vector<VkSubpassDependency> subpassDependencies(2);
        // subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        // subpassDependencies[0].dstSubpass = 0;
        // subpassDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        // subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        // subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        // subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        // subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        // subpassDependencies[1].srcSubpass = 0;
        // subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        // subpassDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        // subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        // subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        // subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        // subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        VkRenderPassCreateInfo renderPassInfo = {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 0;
        renderPassInfo.pDependencies = nullptr;

        VkRenderPass renderPass;
        if (vkCreateRenderPass(m_LogicalDevice, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create render pass!");
        }

        // {
        //     ScopeLock lock(sync);
        //     renderPassMap.insert({info, renderPass});
        // }
        return renderPass;
	}

	VkFramebuffer VulkanDynamicRHI::CreateVkFramebuffer(const VkFramebufferCreateInfo& info)
	{
        VkFramebuffer frameBuffer;
        if (vkCreateFramebuffer(m_LogicalDevice, &info, nullptr, &frameBuffer) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to create framebuffer!");
        }

        return frameBuffer;
	}

    VulkanRHICommandContext::VulkanRHICommandContext(RHICommandPoolRef pool): RHICommandContext(pool)
    {
        this->pool = std::static_pointer_cast<VulkanRHICommandPool>(pool);
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = this->pool->GetHandle();
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(VULKAN_DEVICE, &allocInfo, &handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to allocate command buffer!");
        }
    }

	void VulkanRHICommandContext::BeginCommand()
	{
        vkResetCommandBuffer(handle, 0);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;

        vkBeginCommandBuffer(handle, &beginInfo);
	}

	void VulkanRHICommandContext::EndCommand()
	{
        if (vkEndCommandBuffer(handle) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to end command buffer!");
        }
	}

	void VulkanRHICommandContext::Execute(RHIFenceRef fence, RHISemaphoreRef waitSemaphore, RHISemaphoreRef signalSemaphore)
	{
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
        VkFence signalFence = VK_NULL_HANDLE;

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;

        if (fence != nullptr)
        {
            signalFence = CAST<VulkanRHIFence>(fence)->GetHandle();
        }
        if (waitSemaphore != nullptr)
        {
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &CAST<VulkanRHISemaphore>(waitSemaphore)->GetHandle();
            submitInfo.pWaitDstStageMask = &stage;
        }
        if (signalSemaphore != nullptr)
        {
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &CAST<VulkanRHISemaphore>(signalSemaphore)->GetHandle();
        }

        if (vkQueueSubmit(CAST<VulkanRHIQueue>(pool->GetQueue())->GetHandle(), 1, &submitInfo, signalFence) != VK_SUCCESS)
        {
            LOG_ERROR("Failed to submit command buffer!");
        }
	}

	VulkanRHICommandContextImmediate::VulkanRHICommandContextImmediate()
	{
        fence = VULKAN_RHI->CreateFence(true);
        queue = VULKAN_RHI->GetQueue({ QUEUE_TYPE_GRAPHICS, 0 });
        commandPool = VULKAN_RHI->CreateCommandPool({ queue });
	}


	void VulkanRHICommandContextImmediate::TextureBarrier(const RHITextureBarrier& barrier)
	{
        TextureSubresourceRange range = barrier.subresource;
        if (range.aspect == TEXTURE_ASPECT_NONE) range = barrier.texture->GetDefaultSubresourceRange();

        VkAccessFlags srcAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.srcState);
        VkAccessFlags dstAccessMask = VulkanUtil::ResourceStateToAccessFlags(barrier.dstState);
        VkPipelineStageFlags srcStage = VulkanUtil::AccessFlagsToPipelineStageFlags(srcAccessMask);
        VkPipelineStageFlags dstStage = VulkanUtil::AccessFlagsToPipelineStageFlags(dstAccessMask);

        VkImageMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        memoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        memoryBarrier.oldLayout = VulkanUtil::ResourceStateToImageLayout(barrier.srcState);
        memoryBarrier.newLayout = VulkanUtil::ResourceStateToImageLayout(barrier.dstState);
        memoryBarrier.image = CAST<VulkanRHITexture>(barrier.texture)->GetHandle();
        memoryBarrier.subresourceRange = VulkanUtil::SubresourceToVk(range);
        memoryBarrier.srcAccessMask = srcAccessMask;
        memoryBarrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            handle,
            srcStage, dstStage, 0,
            0, nullptr,
            0, nullptr,
            1, &memoryBarrier);
	}

	void VulkanRHICommandContextImmediate::Flush()
	{
        EndSingleTimeCommand();
	}

	void VulkanRHICommandContextImmediate::BeginSingleTimeCommand()
	{
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = CAST<VulkanRHICommandPool>(commandPool)->GetHandle();
        allocInfo.commandBufferCount = 1;

        vkAllocateCommandBuffers(VULKAN_DEVICE, &allocInfo, &handle);

        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(handle, &beginInfo);
	}

	void VulkanRHICommandContextImmediate::EndSingleTimeCommand()
	{
        // 等待前一次flush执行完成
        fence->Wait();
        if (oldHandle != VK_NULL_HANDLE) vkFreeCommandBuffers(VULKAN_DEVICE, CAST<VulkanRHICommandPool>(commandPool)->GetHandle(), 1, &oldHandle);

        // 提交最新一次的flush
        if (vkEndCommandBuffer(handle) != VK_SUCCESS) {
            LOG_ERROR("Failed to record command buffer!");
        }

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &handle;
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;
        submitInfo.signalSemaphoreCount = 0;
        submitInfo.pSignalSemaphores = nullptr;

        VkResult result = vkQueueSubmit(CAST<VulkanRHIQueue>(queue)->GetHandle(), 1, &submitInfo, CAST<VulkanRHIFence>(fence)->GetHandle());
        if (result != VK_SUCCESS)
        {
            LOG_ERROR("Failed to submit draw command buffer! [%d]", result);
        }

        oldHandle = handle;
	}

	void VulkanRHICommandContextImmediate::GenerateMips(RHITextureRef src)
	{
        //总计生成的mip层数
        //uint32_t mipLevels = src->GetInfo().mipLevels;

        //VkImageSubresourceRange transition = {};
        //transition.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        //    transition.baseMipLevel = 0;
        //transition.levelCount = 1;
        //transition.baseArrayLayer = 0;
        //transition.layerCount = 1;

        //for (uint32_t i = 0; i < src->GetInfo().arrayLayers; i++)
        //{
        //    transition.baseMipLevel = 0;
        //    transition.baseArrayLayer = i;

        //    // 先将后面的层全设置到dst
        //    TextureBarrier(handle,
        //        { src,
        //        RESOURCE_STATE_TRANSFER_SRC, RESOURCE_STATE_TRANSFER_DST,
        //                {TEXTURE_ASPECT_COLOR, 1, mipLevels - 1, transition.baseArrayLayer, transition.layerCount} });

        //    //循环生成各级mip，并将对应层级转到srcLayout
        //    for (uint32_t i = 1; i < mipLevels; i++)    //总共mipLevels级，只需要mipLevels-1次blit
        //    {
        //        BlitTexture(
        //            commandBuffer,
        //            src,
        //            src,
        //            { transition.aspectMask, transition.baseMipLevel, transition.baseArrayLayer, transition.layerCount },
        //            { transition.aspectMask, transition.baseMipLevel + 1, transition.baseArrayLayer, transition.layerCount },
        //            FILTER_TYPE_LINEAR);

        //        // 将生成后的层级设置到src
        //        TextureBarrier(commandBuffer,
        //            { src,
        //            RESOURCE_STATE_TRANSFER_DST, RESOURCE_STATE_TRANSFER_SRC,
        //                    {TEXTURE_ASPECT_COLOR, transition.baseMipLevel + 1, 1, transition.baseArrayLayer, transition.layerCount} });

        //        transition.baseMipLevel++;
        //    }
        //}
	}

}













