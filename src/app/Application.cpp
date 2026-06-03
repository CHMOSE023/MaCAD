#include "app/Application.hpp"

#include "core/Log.hpp"
#include "geometry/Primitives.hpp"
#include "geometry/SketchToShape.hpp"
#include "geometry/Tessellator.hpp"
#include "plugin/ICommand.hpp"
#include "sketch/Sketch.hpp"
#include "sketch/SketchPlane.hpp"

#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif

#include <imgui.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>

namespace macad::app
{ 
    namespace 
    {

        // Adapts lambdas into an ICommand so the app can register built-ins without a
        // bespoke class per action. Providing an undoFn opts the command into the
        // undo stack; omitting it produces a fire-and-forget command.
        class FunctionCommand : public ICommand
        {
        public:
            FunctionCommand(std::string id, std::string label, std::function<void()> fn, std::function<void()> undoFn = {})
                : m_id(std::move(id))
                , m_label(std::move(label))
                , m_fn(std::move(fn))
                , m_undoFn(std::move(undoFn)) 
            {
            }

            std::string id()    const override { return m_id; }
            std::string label() const override { return m_label; }
            bool undoable()     const override { return static_cast<bool>(m_undoFn); }
            void execute()            override { if (m_fn)     m_fn(); }
            void undo()               override { if (m_undoFn) m_undoFn(); }

        private:
            std::string           m_id;
            std::string           m_label;
            std::function<void()> m_fn;
            std::function<void()> m_undoFn;
        }; 

    } 

    Application::~Application() { shutdown(); }

    bool Application::init(int width, int height)
    {
        Log::init();
        MACAD_LOG_INFO("MaCAD starting up");

        if (!glfwInit()) {
            MACAD_LOG_ERROR("glfwInit failed");
            return false;
        }

        // bgfx owns rendering: do not create any GL/GLES context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(width, height, "MaCAD", nullptr, nullptr);
        if (!m_window)
        {
            MACAD_LOG_ERROR("glfwCreateWindow failed");
            glfwTerminate();
            return false;
        }

        render::NativeWindow native;
#ifdef _WIN32
        native.nwh = glfwGetWin32Window(m_window);
        native.ndt = nullptr;
#else
        // Other platforms (X11/Wayland/Cocoa) wire up here in a later iteration.
        MACAD_LOG_WARN("Native window handle not wired for this platform");
#endif

        if (!m_renderer.init(native, width, height))
        {
            return false;
        }
        m_camera.setViewport(width, height);

        if (!m_imgui.init(m_window)) 
        {
            return false;
        }

        registerBuiltinCommands();
        rebuildBox(m_boxDims[0], m_boxDims[1], m_boxDims[2]);

        m_stats.backend = m_renderer.backendName();
        m_running = true;
        return true;
    }

    void Application::registerBuiltinCommands()
    {
        // Demonstrates the command/plugin contract with a built-in, in-process
        // command. A real plugin would register the same way via registerWith().
        m_registry.registerCommand(std::make_shared<FunctionCommand>(
            "macad.geometry.createBox", "Create Box",
            // execute: step the X dimension, rebuild.
            [this] {
                m_prevBoxDims[0] = m_boxDims[0];
                m_prevBoxDims[1] = m_boxDims[1];
                m_prevBoxDims[2] = m_boxDims[2];
                m_boxDims[0] = (m_boxDims[0] >= 3.0) ? 1.0 : m_boxDims[0] + 0.5;
                rebuildBox(m_boxDims[0], m_boxDims[1], m_boxDims[2]);
            },
            // undo: restore the previous dimensions.
            [this] {
                m_boxDims[0] = m_prevBoxDims[0];
                m_boxDims[1] = m_prevBoxDims[1];
                m_boxDims[2] = m_prevBoxDims[2];
                rebuildBox(m_boxDims[0], m_boxDims[1], m_boxDims[2]);
            }));

        // M2: enter/leave the 2D sketch editor. The SketchView owns its sketch
        // and drives editing entirely through ImGui overlays + panels.
        m_registry.registerCommand(std::make_shared<FunctionCommand>(  "macad.sketch.toggle", "Sketch", [this] { m_sketchView.toggle(); }));
    }

    void Application::rebuildBox(double dx, double dy, double dz) 
    {
        const geometry::Shape shape = geometry::Primitives::MakeBox(dx, dy, dz);
        const MeshData data = geometry::Tessellator::Tessellate(shape, 0.05);
        m_mesh.upload(data);
        m_stats.vertexCount = static_cast<std::uint32_t>(data.vertexCount());
        m_stats.triangleCount = static_cast<std::uint32_t>(data.triangleCount());
    }

    void Application::handleCameraInput() 
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse)
        {
            return;
        }
        // In sketch mode the left button drives the sketch (draw/drag/select), so
        // orbit moves to the right button and pan to the middle button; outside
        // sketch mode the left button orbits as before.
        const bool sketching = m_sketchView.active();

        if (!sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) 
        {
            m_camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }

        if (sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            m_camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || (!sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Right)))
        {
            m_camera.pan(io.MouseDelta.x, io.MouseDelta.y);
        }

        if (io.MouseWheel != 0.0f) 
        {
            m_camera.dolly(io.MouseWheel);
        }
    }

    void Application::syncFramebufferSize() 
    {
        int w = 0, h = 0;
        glfwGetFramebufferSize(m_window, &w, &h);

        if (w > 0 && h > 0 && (w != m_renderer.width() || h != m_renderer.height()))
        {
            m_renderer.reset(w, h);
            m_camera.setViewport(w, h);
        }
    }

    void Application::run() 
    {
        double lastTime = glfwGetTime();

        while (m_running && !glfwWindowShouldClose(m_window))
        {
            glfwPollEvents();
            syncFramebufferSize();

            const double now = glfwGetTime();
            const double dt  = now - lastTime;
            lastTime         = now;
            m_stats.fps      = dt > 0.0 ? 1.0 / dt : 0.0;

            m_imgui.beginFrame();

            handleCameraInput();

            // Consume pending operations from the sketch editor.
            if (m_sketchView.hasPendingExtrude())
            {
                onExtrude();
                m_sketchView.clearPendingExtrude();
            }
            if (m_sketchView.hasPendingRevolve())
            {
                onRevolve();
                m_sketchView.clearPendingRevolve();
            }
            // Ctrl+Z / Ctrl+Y undo-redo (when ImGui is not capturing keyboard).
            if (!ImGui::GetIO().WantCaptureKeyboard)
            {
                const bool ctrl = ImGui::GetIO().KeyCtrl;
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
                    m_history.undo();
                }
                if (ctrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
                    m_history.redo();
                }
            }

            // Build lightweight feature info list for the feature tree UI.
            std::vector<ui::FeatureInfo> featureInfos;
            featureInfos.reserve(m_features.size());
            for (const Feature& f : m_features) 
            
            {
                ui::FeatureInfo fi;
                fi.name          = f.name;
                fi.triangleCount = f.mesh ? f.mesh->indexCount() / 3 : 0;
                featureInfos.push_back(fi);
            }
            ui::Panels::draw(m_registry, m_history, m_stats, featureInfos);

            // Sketch overlay needs the camera's view-projection to map the 2D
            // plane to screen space. The same matrix drives picking/dragging.
            const glm::mat4 viewProj = m_camera.projMatrix() * m_camera.viewMatrix();
            m_sketchView.update(viewProj);
            m_sketchView.drawPanels();

            m_renderer.beginFrame(m_camera);

            if (!m_sketchView.active())
            {
                // Draw the demo box only when no extruded features exist yet.
                if (m_features.empty())
                {
                    m_renderer.drawMesh(m_mesh, glm::mat4(1.0f));
                }
                for (const Feature& f : m_features)
                {
                    if (f.mesh && f.mesh->valid()) {
                        m_renderer.drawMesh(*f.mesh, glm::mat4(1.0f));
                    }
                }
            }

            m_imgui.endFrame();
            m_renderer.endFrame();
        }
    }

    void Application::onExtrude()
    {
        const sketch::Sketch&      sk     = m_sketchView.sketch();
        const sketch::SketchPlane& plane  = sk.plane();
        const double               height = m_sketchView.pendingExtrudeHeight();

        MACAD_LOG_INFO("Extruding sketch (height={:.3f}) ...", height);

        const geometry::Shape face = geometry::SketchToShape::buildFace(sk);
        if (face.isNull()) 
        {
            MACAD_LOG_WARN("Extrude: sketch has no closed profile — draw a closed polygon or a circle first");
            return;
        }

        const geometry::Shape solid = geometry::SketchToShape::extrude(face, plane, height);
        if (solid.isNull()) {
            MACAD_LOG_ERROR("Extrude: solid creation failed");
            return;
        }

        const MeshData data = geometry::Tessellator::Tessellate(solid, 0.05);
        if (data.vertexCount() == 0) {
            MACAD_LOG_ERROR("Extrude: tessellation produced an empty mesh");
            return;
        }

        Feature f;
        f.name = "Extrude " + std::to_string(m_features.size() + 1);
        f.mesh = std::make_unique<render::Mesh>();
        f.mesh->upload(data);
        m_features.push_back(std::move(f));

        // Update stats to reflect the newly added solid.
        m_stats.vertexCount   = static_cast<std::uint32_t>(data.vertexCount());
        m_stats.triangleCount = static_cast<std::uint32_t>(data.triangleCount());

        MACAD_LOG_INFO("Extrude done: {} verts, {} tris",
                       data.vertexCount(), data.triangleCount());
    }

    void Application::onRevolve()
    {
        const sketch::Sketch&      sk      = m_sketchView.sketch();
        const sketch::SketchPlane& plane   = sk.plane();
        const double               angle   = m_sketchView.pendingRevolveAngle();
        const bool                 aroundV = m_sketchView.pendingRevolveAroundV();

        MACAD_LOG_INFO("Revolving sketch (angle={:.1f}°, axis={}) ...",
                       angle, aroundV ? "V" : "U");

        const geometry::Shape face = geometry::SketchToShape::buildFace(sk);
        if (face.isNull())
        {
            MACAD_LOG_WARN("Revolve: sketch has no closed profile — draw a closed "
                           "polygon or a circle first");
            return;
        }

        const geometry::Shape solid = geometry::SketchToShape::revolve(face, plane, aroundV, angle);
        if (solid.isNull())
        {
            MACAD_LOG_ERROR("Revolve: solid creation failed");
            return;
        }

        const MeshData data = geometry::Tessellator::Tessellate(solid, 0.05);
        if (data.vertexCount() == 0)
        {
            MACAD_LOG_ERROR("Revolve: tessellation produced an empty mesh");
            return;
        }

        Feature f;
        f.name = "Revolve " + std::to_string(m_features.size() + 1);
        f.mesh = std::make_unique<render::Mesh>();
        f.mesh->upload(data);
        m_features.push_back(std::move(f));

        m_stats.vertexCount   = static_cast<std::uint32_t>(data.vertexCount());
        m_stats.triangleCount = static_cast<std::uint32_t>(data.triangleCount());

        MACAD_LOG_INFO("Revolve done: {} verts, {} tris",
                       data.vertexCount(), data.triangleCount());
    }

    void Application::shutdown() {
        if (m_window) {
            m_imgui.shutdown();
            m_mesh.destroy();
            for (Feature& f : m_features) {
                if (f.mesh) f.mesh->destroy();
            }
            m_features.clear();
            m_renderer.shutdown(); // calls bgfx::shutdown()
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            MACAD_LOG_INFO("MaCAD shut down");
        }
        m_running = false;
    }

}  
