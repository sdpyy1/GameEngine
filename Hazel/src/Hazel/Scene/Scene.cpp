#include "hzpch.h"
#include "Scene.h"
#include "Entity.h"
#include "Hazel/Core/Application.h"
#include "Components.h"
#include "ScriptableEntity.h"
#include "Hazel/Scripting/ScriptEngine.h"
#include "Hazel/Physics/Physics2D.h"
#include "Hazel/Renderer/Renderer.h"
#define GLM_FORCE_DEPTH_ZERO_TO_FE 
#include <glm/glm.hpp>

#include "Entity.h"
#include "examples/imgui_impl_vulkan_with_textures.h"

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
#include <imgui.h>

namespace Hazel {

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 texCoord;
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


	void Scene::updateUniformBuffer(EditorCamera& editorCamera) {
		if (swapChian->GetExtent().width == 0 && (float)swapChian->GetExtent().height == 0) {
			return;
		}
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		ubo->model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo->view = editorCamera.GetViewMatrix();
		ubo->proj = editorCamera.GetProjectionMatrix();
		ubo->proj[1][1] *= -1; // Y轴反转

		uniformBufferSet->RT_Get()->SetData((void*)ubo, sizeof(UniformBufferObject));
	}
	Scene::Scene()
	{
		passCommandBuffer = RenderCommandBuffer::Create("PassCommandBuffer");
		vulkanContext = Application::Get().GetRenderContext().As<VulkanContext>();
		swapChian = Application::Get().GetWindow()->GetSwapChainPtr();
		gBuffershader = Renderer::GetShaderLibrary()->Get("gBuffer").As<VulkanShader>();
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

		ubo = new UniformBufferObject();
		// 顶点创建
		testVertexBuffer = VertexBuffer::Create((void*)vertices.data(), sizeof(vertices[0]) * vertices.size(), VertexBufferUsage::Static);
		indexBuffer = IndexBuffer::Create((void*)indices.data(), sizeof(indices[0]) * indices.size());
		uniformBufferSet = UniformBufferSet::Create(sizeof(UniformBufferObject));

		RenderPassSpecification gBufferPassSpec;
		gBufferPassSpec.Pipeline = GbufferPipeline;
		gBufferPass = RenderPass::Create(gBufferPassSpec);
		TextureSpecification textureSpec;
		textureSpec.Width = 100;
		textureSpec.Height = 100;
		std::filesystem::path path = "assets/textures/texture.jpg";
		texture = Texture2D::Create(textureSpec, path);
		gBufferPass->SetInput(uniformBufferSet, 0);  // 传递实际数据给Shader
		gBufferPass->SetInput(texture, 1);  // 传递实际数据给Shader

	}

	Scene::~Scene()
	{
		delete m_PhysicsWorld;
	}

	void Scene::RenderVukan(EditorCamera & editorCamera) {
		passCommandBuffer->Begin();
		uint32_t flyIndex = Renderer::GetCurrentFrameIndex();
		updateUniformBuffer(editorCamera);

		Renderer::BeginRenderPass(passCommandBuffer,gBufferPass,true);

		Renderer::BindVertData(passCommandBuffer,testVertexBuffer);
		Renderer::BindIndexDataAndDraw(passCommandBuffer,indexBuffer);
		
		Renderer::EndRenderPass(passCommandBuffer);
		passCommandBuffer->End();
		passCommandBuffer->Submit();
	};

	void Image(const Ref<Image2D>& image, const ImVec2& size, const ImVec2& uv0, const ImVec2& uv1, const ImVec4& tint_col = { 1,1,1,1 },
		const ImVec4& border_col = { 0,0,0,0 })
	{
		HZ_CORE_VERIFY(image, "Image is null");

		if (RendererAPI::Current() == RendererAPI::Type::Vulkan)
		{
			Ref<VulkanImage2D> vulkanImage = image.As<VulkanImage2D>();
			const auto& imageInfo = vulkanImage->GetImageInfo();
			if (!imageInfo.ImageView)
				return;
			const auto textureID = ImGui_ImplVulkan_AddTexture(imageInfo.Sampler, imageInfo.ImageView, vulkanImage->GetDescriptorInfoVulkan().imageLayout);
			ImGui::Image(textureID, size, uv0, uv1, tint_col, border_col);
		}
	}

	void Scene::SetViewPortImage()
	{
		Ref<Image2D> finalRenderOutput = gBufferPass->GetPipeline()->GetSpecification().TargetFramebuffer->GetImage(1);

		auto viewportSize = ImGui::GetContentRegionAvail();
		Image(finalRenderOutput, viewportSize, {0, 0}, {1, 1});
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
		//if (m_ViewportWidth == width && m_ViewportHeight == height)
		//	return;

		//m_ViewportWidth = width;
		//m_ViewportHeight = height;

		//// Resize our non-FixedAspectRatio cameras
		//auto view = m_Registry.view<CameraComponent>();
		//for (auto entity : view)
		//{
		//	auto& cameraComponent = view.get<CameraComponent>(entity);
		//	if (!cameraComponent.FixedAspectRatio)
		//		cameraComponent.Camera.SetViewportSize(width, height);
		//}
	}

	Entity Scene::GetPrimaryCameraEntity()
	{
		//auto view = m_Registry.view<CameraComponent>();
		//for (auto entity : view)
		//{
		//	const auto& camera = view.get<CameraComponent>(entity);
		//	if (camera.Primary)
		//		return Entity{entity, this};
		//}
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

	//template<>
	//void Scene::OnComponentAdded<CameraComponent>(Entity entity, CameraComponent& component)
	//{
	//	if (m_ViewportWidth > 0 && m_ViewportHeight > 0)
	//		component.Camera.SetViewportSize(m_ViewportWidth, m_ViewportHeight);
	//}

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
