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
#include "plugin/CommandStack.hpp"

#include <memory>
#include <string>
#include <vector>

struct GLFWwindow;

namespace macad::app
{
    class Application 
    {
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
        void onExtrude();      // called when SketchView has a pending extrude

        // A built solid feature: name + GPU mesh.
        struct Feature 
        {
            std::string                  name;
            std::unique_ptr<render::Mesh> mesh;
        };

        GLFWwindow*      m_window{ nullptr };
        render::Renderer m_renderer;
        render::Camera   m_camera;
        render::Mesh     m_mesh;
        ui::ImGuiLayer   m_imgui;
        ui::SketchView   m_sketchView;
        ui::FrameStats   m_stats;
        PluginRegistry   m_registry;
        CommandStack     m_history;

        double m_boxDims    [3]{ 2.0, 1.5, 1.0 };
        double m_prevBoxDims[3]{ 2.0, 1.5, 1.0 };
        std::vector<Feature> m_features;
        bool m_running{ false };
    };

} 
