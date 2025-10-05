#pragma once

#include "Hazel/Core/Timestep.h"
#include "Hazel/Core/UUID.h"
#include "Hazel/Renderer/EditorCamera.h"
#include "Platform/Vulkan/VulkanShader.h"
#include "entt.hpp"
#include <Hazel/Renderer/RenderContext.h>
#include "Platform/Vulkan/VulkanContext.h"
#include <Hazel/Renderer/IndexBuffer.h>
class b2World;

namespace Hazel {

	class Entity;

	class Scene: public RefCounted
	{
	public:
		void updateUniformBuffer(uint32_t currentImage);
		Scene();
		~Scene();
		
		// 临时调试接口
		void RenderVukan();
		void createDescriptorPool();
		void createDescriptorSets();
		void createGraphicsPipeline();

	private:
		Ref<VulkanContext> vulkanContext;
		VkPipelineLayout pipelineLayout;
		VkPipeline graphicsPipeline;
		VulkanSwapChain *swapChian;
		VkDevice device;
		// UBO
		struct UniformBufferObject {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};
		VkDescriptorPool descriptorPool;
		std::vector<VkDescriptorSet> descriptorSets;
		Ref<VulkanShader> shader;
		UniformBufferObject* ubo;
		Ref<VertexBuffer> testVertexBuffer;
		Ref<IndexBuffer> indexBuffer;
		Ref<UniformBufferSet> uniformBufferSet;
	public:
		static Ref<Scene> Copy(Ref<Scene> other);

		Entity CreateEntity(const std::string& name = std::string());
		Entity CreateEntityWithUUID(UUID uuid, const std::string& name = std::string());
		void DestroyEntity(Entity entity);

		void OnViewportResize(uint32_t width, uint32_t height);

		Entity DuplicateEntity(Entity entity);

		Entity FindEntityByName(std::string_view name);
		Entity GetEntityByUUID(UUID uuid);

		Entity GetPrimaryCameraEntity();

		bool IsRunning() const { return m_IsRunning; }
		bool IsPaused() const { return m_IsPaused; }

		void SetPaused(bool paused) { m_IsPaused = paused; }

		void Step(int frames = 1);

		template<typename... Components>
		auto GetAllEntitiesWith()
		{
			return m_Registry.view<Components...>();
		}
	private:
		template<typename T>
		void OnComponentAdded(Entity entity, T& component);


		void RenderScene(EditorCamera& camera);
	private:
		entt::registry m_Registry;
		uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;
		bool m_IsRunning = false;
		bool m_IsPaused = false;
		int m_StepFrames = 0;

		b2World* m_PhysicsWorld = nullptr;

		std::unordered_map<UUID, entt::entity> m_EntityMap;

		friend class Entity;
		friend class SceneSerializer;
		friend class SceneHierarchyPanel;
	};

}
