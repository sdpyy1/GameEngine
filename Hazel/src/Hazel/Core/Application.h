#pragma once

#include "Hazel/Core/Base.h"
namespace Hazel
{
    class SceneManager;
    class RendererManager;
    class Event;
    class WindowMinimizeEvent;
    class WindowCloseEvent;
    class WindowResizeEvent;
    class Window;
    class WindowsWindow;
    class RenderContext;
    struct ApplicationCommandLineArgs
    {
        int Count = 0;
        char** Args = nullptr;

        const char* operator[](int index) const
        {
            ASSERT(index < Count);
            return Args[index];
        }
    };

    struct ApplicationSpecification
    {
        std::string Name = "Hazel Application";
        ApplicationCommandLineArgs CommandLineArgs;
    };

    class Application
    {
    public:
        Application(const ApplicationSpecification& specification);
        virtual ~Application() = default;

        void Tick();
        void OnEvent(Event& e);
        void Close();

        bool isRunning() const { return m_Running; }
        bool isMinimized() const { return m_Minimized; }

        Ref<WindowsWindow>& GetWindow();
        Ref<RenderContext> GetRenderContext();
        const ApplicationSpecification& GetSpecification() const { return m_Specification; }
        uint32_t GetCurrentFrameIndex() const { return m_CurrentFrameIndex; }

        static Application& Get() { return *s_Instance; }
        static std::shared_ptr<SceneManager> GetSceneManager() { return Get().m_SceneManager; }
        static std::shared_ptr<RendererManager> GetRendererManager() { return Get().m_RendererManager; }

    private:
        bool OnWindowClose(WindowCloseEvent& e);
        bool OnWindowResize(WindowResizeEvent& e);
        bool OnWindowMinimize(WindowMinimizeEvent& e);
        float GetTimePreFrame();

    private:
        ApplicationSpecification m_Specification;
        static Application* s_Instance;

        bool m_Running = true;
        bool m_Minimized = false;
        float m_LastFrameTime = 0.0f;

        // Context
        Ref<Window> m_Window;
        std::shared_ptr<RendererManager> m_RendererManager;
        std::shared_ptr<SceneManager> m_SceneManager;

        uint32_t m_CurrentFrameIndex = 0;
    };
}