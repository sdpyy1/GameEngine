#pragma once
#include "Hazel/Core/Base.h"
#include "Hazel/Core/Definations.h"
#include "Hazel/Window/WindowManager.h"
#include "Hazel/Renderer/RenderSystem/RenderSystem.h"
namespace GameEngine
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
        std::string Name = "GameEngine Application";
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
        Ref<Window> m_GLFWWindow;
        std::shared_ptr<RendererManager> m_RendererManager;
        std::shared_ptr<SceneManager> m_SceneManager;

        uint32_t m_CurrentFrameIndex = 0;



    // New
    public:
        static std::shared_ptr<WindowManager> GetWindowManager() { return Get().m_WindowManager; }
        static std::shared_ptr<RenderSystem> GetRenderSystem() { return Get().m_RenderSystem; }
        static uint32_t GetFrameIndex() { return Get().m_CurrentFrameIndex; }
    private:
        std::shared_ptr<WindowManager> m_WindowManager;
        std::shared_ptr<RenderSystem> m_RenderSystem;


    };
}