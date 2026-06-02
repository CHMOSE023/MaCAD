#pragma once

// Top-level application: owns the window, the renderer, the UI layer, the
// plugin registry, and the demo geometry. Orchestrates init -> main loop ->
// shutdown. This is the only place all module layers meet.

#include "render/Camera.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"
#include "ui/ImGuiLayer.hpp"
#include "ui/Panels.hpp"
#include "ui/SketchView.hpp"
#include "plugin/PluginRegistry.hpp"

struct GLFWwindow;

namespace macad::app {

    class Application {
    public:
        Application() = default;
        ~Application();

        bool init(int width = 1280, int height = 720);
        void run();
        void shutdown();

    private:
        void registerBuiltinCommands();
        void rebuildBox(double dx, double dy, double dz);
        void handleCameraInput();
        void syncFramebufferSize();

        GLFWwindow* m_window{ nullptr };
        render::Renderer m_renderer;
        render::Camera m_camera;
        render::Mesh m_mesh;
        ui::ImGuiLayer m_imgui;
        ui::SketchView m_sketchView;
        ui::FrameStats m_stats;
        PluginRegistry m_registry;

        double m_boxDims[3]{ 2.0, 1.5, 1.0 };
        bool m_running{ false };
    };

} 
