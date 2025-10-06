#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Hazel/Core/Application.h"
#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Physics/Physics2D.h"
#include "Hazel/Renderer/Renderer.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_FE   // 把深度值范围设置为[0, 1]，而不是OpenGL的[-1, 1]
#include <glm/glm.hpp>

#include "Entity.h"

// Box2D
#include "box2d/b2_world.h"
#include "box2d/b2_body.h"
#include "box2d/b2_fixture.h"
#include "box2d/b2_polygon_shape.h"
#include "box2d/b2_circle_shape.h"
#include <Platform/Vulkan/VulkanShader.h>
#include <Platform/Vulkan/VulkanVertexBuffer.h>
#include <Platform/Vulkan/VulkanIndexBuffer.h>
#include <Platform/Vulkan/VulkanUniformBuffer.h>
#include <Platform/Vulkan/VulkanTexture.h>
#include <Platform/Vulkan/VulkanFramebuffer.h>
#include <Platform/Vulkan/VulkanPipeline.h>

namespace Hazel {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;

		// 顶点描述 VkVertexInputBindingDescription
		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0; // 绑定索引 如果把顶点数据放到多个缓冲区，就需要多个绑定描述
			bindingDescription.stride = sizeof(Vertex); // 每个顶点的字节数
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // 表示每个顶点对应一个数据条目（实例化时用另外一个参数）

			return bindingDescription;
		}

		// 属性描述（位置、颜色） VkVertexInputAttributeDescription
		static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

			attributeDescriptions[0].binding = 0; // 绑定索引，必须和bindingDescription.binding一致
			attributeDescriptions[0].location = 0; // 位置location，对应顶点着色器的layout(location = 0)
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3格式
			attributeDescriptions[0].offset = offsetof(Vertex, pos); // 位置数据在结构体中的偏移

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
			return attributeDescriptions;
		}
	};
	const std::vector<Vertex> vertices = {
				{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
		{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
		{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
		{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
		{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},


	};
	const std::vector<uint16_t> indices = {
		0, 1, 2, 2, 3, 0,
		4, 5, 6, 6, 7, 4
	};


	void Scene::updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		ubo->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->proj = glm::perspective(glm::radians(45.0f), swapChian->GetExtent().width / (float)swapChian->GetExtent().height, 0.1f, 10.0f);
		ubo->proj[1][1] *= -1;
		uniformBufferSet->Set_Data(currentImage, (void*)ubo, sizeof(UniformBufferObject));
	}
	Scene::Scene()
	{
		vulkanContext = Application::Get().GetRenderContext().As<VulkanContext>();
		swapChian = Application::Get().GetWindow()->GetSwapChainPtr();
		finalColorShader = Renderer::GetShaderLibrary()->Get("finalColor").As<VulkanShader>();
		gBuffershader = Renderer::GetShaderLibrary()->Get("gBuffer").As<VulkanShader>();
		//TextureSpecification default2DTexture;
		//default2DTexture.Format = ImageFormat::RGBA;
		//default2DTexture.Width = 800;
		//default2DTexture.Height = 600;
		//default2DTexture.SamplerWrap = TextureWrap::Repeat;
		//default2DTexture.SamplerFilter = TextureFilter::Linear;
		//default2DTexture.GenerateMips = true;
		//default2DTexture.Storage = false;
		//default2DTexture.StoreLocally = false;
		//// 调试名称（可选，便于调试时识别）
		//default2DTexture.DebugName = "Texture";
		//texture = Texture2D::Create(default2DTexture, std::filesystem::path("assets/textures/texture.jpg"));



		// FBO创建好后，Vulkan会自动创建附件图片、深度图片、VkRenderPass
		FramebufferTextureSpecification positionSpec(ImageFormat::RGBA16F);	
		FramebufferTextureSpecification colorSpec(ImageFormat::RGBA16F);	
		FramebufferTextureSpecification depthSpec(ImageFormat::DEPTH32F);
		FramebufferAttachmentSpecification attachmentSpec;
		attachmentSpec.Attachments = { positionSpec,colorSpec,depthSpec };
		FramebufferSpecification framebufferSpec;
		framebufferSpec.Attachments = attachmentSpec;
		framebufferSpec.DebugName = "GBuffer";
		Ref<Framebuffer> GbufferFBO = Framebuffer::Create(framebufferSpec);

		positionAttachment = GbufferFBO->GetImage(0);
		HZ_CORE_WARN("Gbuffer 有{}个附件", GbufferFBO->GetColorAttachmentCount());

		// Pipeline 测试
		VertexBufferElement position(ShaderDataType::Float3, "position");
		VertexBufferElement color(ShaderDataType::Float3, "color");
		VertexBufferElement texCoord(ShaderDataType::Float2, "texCoord");
		PipelineSpecification pSpec;
		pSpec.BackfaceCulling = false;
		pSpec.Layout = { position ,color,texCoord };
		pSpec.Shader = gBuffershader;   // TODO：这里要修改为GbuFFer自己的shader
		pSpec.TargetFramebuffer = GbufferFBO;
		pSpec.DebugName = "GbufferPipeline";
		GbufferPipeline = Pipeline::Create(pSpec);


		device = vulkanContext->GetCurrentDevice()->GetVulkanDevice();
		createFinalColorPipeline();
		ubo = new UniformBufferObject();
		// 顶点创建
		testVertexBuffer = VertexBuffer::Create((void*)vertices.data(), sizeof(vertices[0]) * vertices.size(), VertexBufferUsage::Static);
		indexBuffer = IndexBuffer::Create((void*)indices.data(), sizeof(indices[0]) * indices.size());
		uniformBufferSet = UniformBufferSet::Create(sizeof(UniformBufferObject));

		createDescriptorPool();
		createDescriptorSets();
		createFinalColorSets();
	}

	Scene::~Scene()
	{
		delete m_PhysicsWorld;
	}

	void Scene::RenderVukan() {
	
		// 交给APP完成的事情
		// 1. 获取下一帧图片ID
		// 2. 重置命令缓冲区
		// 3. 开始记录命令

		// 这里完成的
		// 4. 绑定Pass
		// 5. 绑定PipleLine
		// 6. 更新UBO
		// 7. 绑定描述符集（资源）
		// 8. 绘图API
		// 9. 结束Pass
		// 10. 结束命令记录
		Scene* instance = this;
		Renderer::Submit([instance]() mutable {

			uint32_t flyIndex = instance->swapChian->GetCurrentBufferIndex();
			VkCommandBuffer commandBuffer = instance->swapChian->GetCurrentDrawCommandBuffer();
			//vkResetCommandBuffer(commandBuffer, /*VkCommandBufferResetFlagBits*/ 0);
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}
			VkRenderPass gbufferPass = instance->GbufferPipeline->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>()->GetRenderPass();
			Ref<VulkanFramebuffer> gbufferfbo = instance->GbufferPipeline->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>();
			VkRenderPassBeginInfo gbufferPassInfo{};
			gbufferPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			gbufferPassInfo.renderPass = gbufferPass;
			gbufferPassInfo.framebuffer = gbufferfbo->GetVulkanFramebuffer();
			gbufferPassInfo.renderArea.offset = { 0, 0 };
			gbufferPassInfo.renderArea.extent = VkExtent2D{ gbufferfbo->GetWidth(),gbufferfbo->GetHeight() };
			gbufferPassInfo.clearValueCount = gbufferfbo->GetColorAttachmentCount()+1; // +1表示深度纹理也清除？
			gbufferPassInfo.pClearValues = gbufferfbo->GetVulkanClearValues().data();

			vkCmdBeginRenderPass(commandBuffer, &gbufferPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			instance->updateUniformBuffer(flyIndex);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->GbufferPipeline.As<VulkanPipeline>()->GetVulkanPipeline());

			VkBuffer vertexBuffers[] = { instance->testVertexBuffer.As<VulkanVertexBuffer>()->GetVulkanBuffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, instance->indexBuffer.As<VulkanIndexBuffer>()->GetVulkanBuffer(), 0, VK_INDEX_TYPE_UINT16);
			VkViewport viewport{};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(gbufferfbo->GetWidth());
			viewport.height = static_cast<float>(gbufferfbo->GetHeight());
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
			VkRect2D scissor{};
			scissor.offset = { 0, 0 };
			scissor.extent = instance->swapChian->GetExtent();
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->GbufferPipeline.As<VulkanPipeline>()->GetVulkanPipelineLayout(), 0, 1, &instance->descriptorSets[flyIndex], 0, nullptr);
			vkCmdDrawIndexed(
				commandBuffer,        // 目标命令缓冲区
				static_cast<uint32_t>(indices.size()), // 索引总数（36个）
				1,                    // 绘制的实例数量（1个立方体，不实例化）
				0,                    // 索引偏移（从第0个索引开始）
				0,                    // 顶点偏移（每个索引对应的顶点索引 + 此值，0表示不偏移）
				0                     // 实例偏移（实例化时用，0表示不偏移）
			);
			vkCmdEndRenderPass(commandBuffer);



			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = instance->swapChian->GetRenderPass();
			renderPassInfo.framebuffer = instance->swapChian->GetCurrentFramebuffer();
			renderPassInfo.renderArea.offset = { 0, 0 };

			renderPassInfo.renderArea.extent = instance->swapChian->GetExtent();

			VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor;

			vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->graphicsPipeline);

			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(instance->swapChian->GetExtent().width);
			viewport.height = static_cast<float>(instance->swapChian->GetExtent().height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);


			scissor.offset = { 0, 0 };
			scissor.extent = instance->swapChian->GetExtent();
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, instance->pipelineLayout, 0, 1, &instance->finaldescriptorSets[flyIndex], 0, nullptr);

			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			vkCmdEndRenderPass(commandBuffer);

			if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}

		});
	
		// APP后续
		// 11. 提交命令
		// 12. 呈现图片
	
	
	};

	void Scene::createDescriptorPool() {
		// 定义两个 Pass 所需的所有描述符类型和数量
		std::vector<VkDescriptorPoolSize> poolSizes;

		// 1. Pass 1 所需：UNIFORM_BUFFER（每个帧 1 个）
		VkDescriptorPoolSize uniformBufferPoolSize{};
		uniformBufferPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferPoolSize.descriptorCount = static_cast<uint32_t>(Renderer::GetConfig().FramesInFlight);
		poolSizes.push_back(uniformBufferPoolSize);

		// 2. Pass 2 所需：COMBINED_IMAGE_SAMPLER（每个帧 1 个）
		VkDescriptorPoolSize imageSamplerPoolSize{};
		imageSamplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageSamplerPoolSize.descriptorCount = static_cast<uint32_t>(Renderer::GetConfig().FramesInFlight);
		poolSizes.push_back(imageSamplerPoolSize);

		// 描述符池创建信息
		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());  // 现在有 2 种类型
		poolInfo.pPoolSizes = poolSizes.data();
		// 最大描述符集数量：两个 Pass 各 FramesInFlight 个，总和是 2*FramesInFlight
		poolInfo.maxSets = 2 * static_cast<uint32_t>(Renderer::GetConfig().FramesInFlight);

		// 可选：允许池在描述符集被释放后复用内存（建议开启，更灵活）
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	void Scene::createFinalColorSets() {
		std::vector<VkDescriptorSetLayout> layouts(Renderer::GetConfig().FramesInFlight, *finalColorShader->GetDescriptorSetLayout());
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::GetConfig().FramesInFlight);
		allocInfo.pSetLayouts = layouts.data();

		// 并行运行的每帧都需要一个描述符集，防止互相影响
		finaldescriptorSets.resize(Renderer::GetConfig().FramesInFlight);
		if (vkAllocateDescriptorSets(device, &allocInfo, finaldescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			// 描述符集的写入操作
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = finaldescriptorSets[i];  // 要更新的描述符集
			descriptorWrites[0].dstBinding = 0; // 绑定点，对应着色器layout(binding = 0)
			descriptorWrites[0].dstArrayElement = 0; // 数组的第几个元素，这里不是数组所以是0
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[0].descriptorCount = 1; // 绑定点是一个数组时，这里就不是1了
			descriptorWrites[0].pImageInfo = &GbufferPipeline->GetSpecification().TargetFramebuffer.As<VulkanFramebuffer>()->GetImage(0).As<VulkanImage2D>()->GetDescriptorInfoVulkan(); // 资源信息

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void Scene::createDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(Renderer::GetConfig().FramesInFlight, *gBuffershader->GetDescriptorSetLayout());
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(Renderer::GetConfig().FramesInFlight);
		allocInfo.pSetLayouts = layouts.data();

		// 并行运行的每帧都需要一个描述符集，防止互相影响
		descriptorSets.resize(Renderer::GetConfig().FramesInFlight);
		if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < Renderer::GetConfig().FramesInFlight; i++) {
			// 实际的ubo对象
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBufferSet->Get(i).As<VulkanUniformBuffer>()->GetVkBuffer();
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);
			// 实际的纹理对象
			//VkDescriptorImageInfo imageInfo{};
			//imageInfo = texture.As<VulkanTexture2D>()->GetDescriptorInfoVulkan();

			// 描述符集的写入操作
			std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];  // 要更新的描述符集
			descriptorWrites[0].dstBinding = 0; // 绑定点，对应着色器layout(binding = 0)
			descriptorWrites[0].dstArrayElement = 0; // 数组的第几个元素，这里不是数组所以是0
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1; // 绑定点是一个数组时，这里就不是1了
			descriptorWrites[0].pBufferInfo = &uniformBufferSet->Get(i).As<VulkanUniformBuffer>()->GetDescriptorBufferInfo(); // 资源信息

			vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}
	void Scene::createFinalColorPipeline()
	{
		// Shader信息
		VkShaderModule vertShaderModule = finalColorShader->GetVertShaderModule();
		VkShaderModule fragShaderModule = finalColorShader->GetFragShaderModule();
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// 顶点信息
		// 顶点信息：关键修改！禁用外部顶点输入
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		// 以下两行是核心修改：不使用任何顶点绑定和属性
		vertexInputInfo.vertexBindingDescriptionCount = 0;  // 无顶点绑定
		vertexInputInfo.pVertexBindingDescriptions = nullptr;
		vertexInputInfo.vertexAttributeDescriptionCount = 0;  // 无顶点属性
		vertexInputInfo.pVertexAttributeDescriptions = nullptr;


		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_NONE;
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;  // 可以指定多个描述符集布局，在shader中通过layout(set = x)指定使用哪个
		pipelineLayoutInfo.pSetLayouts = finalColorShader->GetDescriptorSetLayout();

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = swapChian->GetRenderPass();
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}

		//vkDestroyShaderModule(device, fragShaderModule, nullptr);
		//vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}





	template<typename... Component>
	static void CopyComponent(entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		([&]()
		{
			auto view = src.view<Component>();
			for (auto srcEntity : view)
			{
				entt::entity dstEntity = enttMap.at(src.get<IDComponent>(srcEntity).ID);

				auto& srcComponent = src.get<Component>(srcEntity);
				dst.emplace_or_replace<Component>(dstEntity, srcComponent);
			}
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponent(ComponentGroup<Component...>, entt::registry& dst, entt::registry& src, const std::unordered_map<UUID, entt::entity>& enttMap)
	{
		CopyComponent<Component...>(dst, src, enttMap);
	}

	template<typename... Component>
	static void CopyComponentIfExists(Entity dst, Entity src)
	{
		([&]()
		{
			if (src.HasComponent<Component>())
				dst.AddOrReplaceComponent<Component>(src.GetComponent<Component>());
		}(), ...);
	}

	template<typename... Component>
	static void CopyComponentIfExists(ComponentGroup<Component...>, Entity dst, Entity src)
	{
		CopyComponentIfExists<Component...>(dst, src);
	}

	Ref<Scene> Scene::Copy(Ref<Scene> other)
	{
		Ref<Scene> newScene = Ref<Scene>::Create();

		newScene->m_ViewportWidth = other->m_ViewportWidth;
		newScene->m_ViewportHeight = other->m_ViewportHeight;

		auto& srcSceneRegistry = other->m_Registry;
		auto& dstSceneRegistry = newScene->m_Registry;
		std::unordered_map<UUID, entt::entity> enttMap;

		// Create entities in new scene
		auto idView = srcSceneRegistry.view<IDComponent>();
		for (auto e : idView)
		{
			UUID uuid = srcSceneRegistry.get<IDComponent>(e).ID;
			const auto& name = srcSceneRegistry.get<TagComponent>(e).Tag;
			Entity newEntity = newScene->CreateEntityWithUUID(uuid, name);
			enttMap[uuid] = (entt::entity)newEntity;
		}

		// Copy components (except IDComponent and TagComponent)
		CopyComponent(AllComponents{}, dstSceneRegistry, srcSceneRegistry, enttMap);

		return newScene;
	}

	Entity Scene::CreateEntity(const std::string& name)
	{
		return CreateEntityWithUUID(UUID(), name);
	}

	Entity Scene::CreateEntityWithUUID(UUID uuid, const std::string& name)
	{
		Entity entity = { m_Registry.create(), this };
		entity.AddComponent<IDComponent>(uuid);
		entity.AddComponent<TransformComponent>();
		auto& tag = entity.AddComponent<TagComponent>();
		tag.Tag = name.empty() ? "Entity" : name;

		m_EntityMap[uuid] = entity;

		return entity;
	}

	void Scene::DestroyEntity(Entity entity)
	{
		m_EntityMap.erase(entity.GetUUID());
		m_Registry.destroy(entity);
	}

	void Scene::OnViewportResize(uint32_t width, uint32_t height)
	{
		if (m_ViewportWidth == width && m_ViewportHeight == height)
			return;

		m_ViewportWidth = width;
		m_ViewportHeight = height;

		// Resize our non-FixedAspectRatio cameras
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			auto& cameraComponent = view.get<CameraComponent>(entity);
			if (!cameraComponent.FixedAspectRatio)
				cameraComponent.Camera.SetViewportSize(width, height);
		}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		auto view = m_Registry.view<CameraComponent>();
		for (auto entity : view)
		{
			const auto& camera = view.get<CameraComponent>(entity);
			if (camera.Primary)
				return Entity{entity, this};
		}
		return {};
	}

	void Scene::Step(int frames)
	{
		m_StepFrames = frames;
	}

	Entity Scene::DuplicateEntity(Entity entity)
	{
		// Copy name because we're going to modify component data structure
		std::string name = entity.GetName();
		Entity newEntity = CreateEntity(name);
		CopyComponentIfExists(AllComponents{}, newEntity, entity);
		return newEntity;
	}

	Entity Scene::FindEntityByName(std::string_view name)
	{
		auto view = m_Registry.view<TagComponent>();
		for (auto entity : view)
		{
			const TagComponent& tc = view.get<TagComponent>(entity);
			if (tc.Tag == name)
				return Entity{ entity, this };
		}
		return {};
	}

	Entity Scene::GetEntityByUUID(UUID uuid)
	{
		// TODO(Yan): Maybe should be assert
		if (m_EntityMap.find(uuid) != m_EntityMap.end())
			return { m_EntityMap.at(uuid), this };

		return {};
	}




	void Scene::RenderScene(EditorCamera& camera)
	{
		
	}
  
  template<typename T>
	void Scene::OnComponentAdded(Entity entity, T& component)
	{
		static_assert(sizeof(T) == 0);
	}

	template<>
	void Scene::OnComponentAdded<IDComponent>(Entity entity, IDComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TransformComponent>(Entity entity, TransformComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	{
		if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
			component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	}

	template<>
	void Scene::OnComponentAdded<ScriptComponent>(Entity entity, ScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<SpriteRendererComponent>(Entity entity, SpriteRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleRendererComponent>(Entity entity, CircleRendererComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TagComponent>(Entity entity, TagComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<NativeScriptComponent>(Entity entity, NativeScriptComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<Rigidbody2DComponent>(Entity entity, Rigidbody2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<BoxCollider2DComponent>(Entity entity, BoxCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<CircleCollider2DComponent>(Entity entity, CircleCollider2DComponent& component)
	{
	}

	template<>
	void Scene::OnComponentAdded<TextComponent>(Entity entity, TextComponent& component)
	{
	}

}
