#pragma once

#include "Hazel/Renderer/RenderCommand.h"

#include "Hazel/Renderer/OrthographicCamera.h"
#include "Hazel/Renderer/Shader.h"
#include "GraphicsContext.h"
#include "Hazel/Core/Application.h"
#include "RendererConfig.h"
#include <Hazel/Core/RenderThread.h>
#include "RenderCommandQueue.h"
namespace Hazel {

	class Renderer
	{
	public:
		static RendererConfig& GetConfig();
		static void Init();
		static void Shutdown();
		
		static void OnWindowResize(uint32_t width, uint32_t height);

		static void BeginScene(OrthographicCamera& camera);
		static void EndScene();

		static void Submit_old(const Ref_old<Shader>& shader, const Ref_old<VertexArray>& vertexArray, const glm::mat4& transform = glm::mat4(1.0f));
		static void SwapQueues();
		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();
		static RendererAPI::APIType GetAPI() { return RendererAPI::GetAPI(); }
		static void RenderThreadFunc(RenderThread* renderThread);
		static void WaitAndRender(RenderThread* renderThread);
		static uint32_t GetRenderQueueIndex();


		template<typename FuncT>
		static void Submit(FuncT&& func)  // FuncT&&���Խ��ո������͵ĺ������󣬰���lambda���ʽ������ָ���
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr; // �Ѵ����void*ָ��ת����FuncT���͵�ָ��
				(*pFunc)(); // ���ú�������

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
				};
			auto storageBuffer = GetRenderCommandQueue().Allocate(renderCmd, sizeof(func)); 
			new (storageBuffer) FuncT(std::forward<FuncT>(func));// ��������
		}
		template<typename FuncT>
		static void SubmitResourceFree(FuncT&& func)
		{
			auto renderCmd = [](void* ptr) {
				auto pFunc = (FuncT*)ptr;
				(*pFunc)();

				// NOTE: Instead of destroying we could try and enforce all items to be trivally destructible
				// however some items like uniforms which contain std::strings still exist for now
				// static_assert(std::is_trivially_destructible_v<FuncT>, "FuncT must be trivially destructible");
				pFunc->~FuncT();
				};

			if (RenderThread::IsCurrentThreadRT())
			{
				const uint32_t index = Renderer::RT_GetCurrentFrameIndex();
				auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
				new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
			}
			else
			{
				const uint32_t index = Renderer::GetCurrentFrameIndex();
				Submit([renderCmd, func, index]()
					{
						auto storageBuffer = GetRenderResourceReleaseQueue(index).Allocate(renderCmd, sizeof(func));
						new (storageBuffer) FuncT(std::forward<FuncT>((FuncT&&)func));
					});
			}
		}
		static RenderCommandQueue& GetRenderCommandQueue();
		static RenderCommandQueue& GetRenderResourceReleaseQueue(uint32_t index);

	private:
		struct SceneData
		{
			glm::mat4 ViewProjectionMatrix;
		};
		static Scope<SceneData> s_SceneData ;


	};
}
