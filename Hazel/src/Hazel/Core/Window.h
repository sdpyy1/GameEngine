#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Renderer/RenderContext.h"
#include <sstream>
#include "Platform/Vulkan/VulkanSwapChain.h"
namespace Hazel {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
		bool VSync;
		WindowProps(const std::string& title = "Hazel Engine",
			        uint32_t width = 800,
			        uint32_t height = 600, bool VSync = false)
			: Title(title), Width(width), Height(height), VSync(VSync)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window: public RefCounted
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;
		virtual void SetTitle(const std::string& title) = 0;

		virtual ~Window() = default;

		virtual void OnUpdate() = 0;

		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		// Window attributes
		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
		virtual void SetVSync(bool enabled) = 0;
		virtual bool IsVSync() const = 0;
		void SwapBuffers(){m_SwapChain->Present();}
		virtual void* GetNativeWindow() const = 0;
		virtual Ref<RenderContext> GetRenderContext() { return m_RenderContext; }
		VulkanSwapChain& GetSwapChain() { return *m_SwapChain; };
		VulkanSwapChain* GetSwapChainPtr() { return m_SwapChain; };

		static Ref<Window> Create(const WindowProps& props = WindowProps());
	protected:
		Ref<RenderContext> m_RenderContext;
		VulkanSwapChain* m_SwapChain;

	};

}
