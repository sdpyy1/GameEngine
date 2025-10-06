#pragma once

#include "Hazel/Renderer/RendererAPI.h"

#include "Hazel/Renderer/Shader.h"
#include "Hazel/Renderer/RenderContext.h"
#include "Hazel/Core/Application.h"
#include "RendererConfig.h"
#include <Hazel/Core/RenderThread.h>
#include "RenderCommandQueue.h"
#include "RendererCapabilities.h"
#include "IndexBuffer.h"
namespace Hazel {
	class ShaderLibrary;

	// ������������ĵ��Ⱥ���Ⱦ��صĳ�ʼ��Դ
	class Renderer
	{
	public:
		static RendererConfig& GetConfig();
		static void Init();
		static void Shutdown();
		static Ref<ShaderLibrary> GetShaderLibrary();

		static void BeginPass(Ref<RenderPass> renderPass);
		static void EndPass();
		static void SwapQueues(); // ����Buffer�����
		static void BindVertData(Ref<VertexBuffer> testVertexBuffer);
		static void BindIndexData(Ref<IndexBuffer> indexBuffer);
		static RendererAPI::Type Current() { return RendererAPI::Current(); }
		static void RenderThreadFunc(RenderThread* renderThread);
		static void WaitAndRender(RenderThread* renderThread);
		static uint32_t GetRenderQueueIndex();

		static void RT_BeginFrame();
		static void RT_EndFrame();

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
		static RendererCapabilities& GetCapabilities();
		static uint32_t GetCurrentFrameIndex();
		static uint32_t RT_GetCurrentFrameIndex();
	private:

	};
}
