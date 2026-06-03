#pragma once

#include "render/Camera.hpp"
#include "render/Mesh.hpp"
#include "render/Renderer.hpp"
#include "ui/ImGuiLayer.hpp"
#include "ui/Panels.hpp"
#include "ui/SketchView.hpp"
#include "plugin/PluginRegistry.hpp"
#include "plugin/CommandStack.hpp"
#include "core/ParameterTable.hpp"
#include "core/AsmTypes.hpp"
#include "sketch/Sketch.hpp"

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
        // ---- Command registration / init helpers -------------------------
        void registerBuiltinCommands();
        void rebuildBox(double dx, double dy, double dz);
        void handleCameraInput();
        void syncFramebufferSize();
        void onExtrude();
        void onRevolve();

        // ---- Feature model -----------------------------------------------
        enum class FeatureKind { Extrude, Revolve };

        struct Feature
        {
            std::string                   name;
            std::unique_ptr<render::Mesh> mesh;

            // Rebuild recipe (M3/M4).
            FeatureKind        kind{ FeatureKind::Extrude };
            sketch::Sketch     sketchSnap;
            std::string        param{ "1.0" };
            bool               revolveAroundV{ false };

            // World transform (M5).
            FeatureTransform   xform;
            glm::mat4          worldMatrix{ 1.0f };

            // Local-space AABB (computed after tessellation, used by solver).
            glm::vec3          aabbMin{ 0.0f };
            glm::vec3          aabbMax{ 0.0f };
        };

        // ---- Geometry helpers -------------------------------------------
        bool rebuildFeature(Feature& f);
        void updateTransform(Feature& f);
        glm::mat4 buildMatrix(const FeatureTransform& xf) const;

        // ---- M4 recompute -----------------------------------------------
        void recompute();

        // ---- M5 assembly ------------------------------------------------
        void solveAssembly();        // legacy feature-level positioning
        void solveMates();           // component instances + mate solver
        glm::mat4 buildMatrixNumeric(const Component& c) const;

        // ---- Members ----------------------------------------------------
        GLFWwindow*      m_window{ nullptr };
        render::Renderer m_renderer;
        render::Camera   m_camera;
        render::Mesh     m_mesh;
        ui::ImGuiLayer   m_imgui;
        ui::SketchView   m_sketchView;
        ui::FrameStats   m_stats;
        PluginRegistry   m_registry;
        CommandStack     m_history;
        ParameterTable   m_params;

        double m_boxDims    [3]{ 2.0, 1.5, 1.0 };
        double m_prevBoxDims[3]{ 2.0, 1.5, 1.0 };

        std::vector<Feature>       m_features;
        std::vector<AsmConstraint> m_constraints;
        int                        m_selectedFeature{ -1 };

        // M5 full assembly: instances + mates + solver status.
        std::vector<Component>     m_components;
        std::vector<Mate>          m_mates;
        AssemblyStatus             m_asmStatus;
        int                        m_selectedComponent{ -1 };

        bool m_running{ false };
    };

}
