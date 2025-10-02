#pragma once

#include "Hazel/Core/Base.h"
#include "Hazel/Events/Event.h"
#include "Hazel/Renderer/GraphicsContext.h"
#include <sstream>
#include "Platform/Vulkan/VulkanSwapChain.h"
namespace Hazel {

	struct WindowProps
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;

		WindowProps(const std::string& title = "Hazel Engine",
			        uint32_t width = 1600,
			        uint32_t height = 900)
			: Title(title), Width(width), Height(height)
		{
		}
	};

	// Interface representing a desktop system based Window
	class Window
	{
	public:
		using EventCallbackFn = std::function<void(Event&)>;

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
		virtual Ref_old<RenderContext> GetRenderContext() { return m_RenderContext; }
		VulkanSwapChain& GetSwapChain() { return *m_SwapChain; };

		static Scope<Window> Create_old(const WindowProps& props = WindowProps());
	protected:
		Ref_old<RenderContext> m_RenderContext;
		VulkanSwapChain* m_SwapChain;

	};

}
