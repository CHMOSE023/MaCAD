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
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <numbers>

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

            // Build lightweight feature info + transform mirrors for the UI.
            std::vector<ui::FeatureInfo>     featureInfos;
            std::vector<FeatureTransform>    transformMirrors;
            featureInfos.reserve(m_features.size());
            transformMirrors.reserve(m_features.size());
            for (const Feature& f : m_features)
            {
                ui::FeatureInfo fi;
                fi.name          = f.name;
                fi.triangleCount = f.mesh ? f.mesh->indexCount() / 3 : 0;
                featureInfos.push_back(fi);
                transformMirrors.push_back(f.xform);
            }

            const unsigned dirty = ui::Panels::draw(
                m_registry, m_history, m_stats,
                featureInfos, m_params,
                m_selectedFeature,
                transformMirrors,
                m_constraints);

            // Write back edited transforms.
            for (int i = 0; i < static_cast<int>(m_features.size()); ++i)
                m_features[i].xform = transformMirrors[i];

            if (dirty & ui::kDirtyParams)
                recompute();
            if (dirty & ui::kDirtyTransforms) {
                for (Feature& f : m_features) updateTransform(f);
            }
            if (dirty & (ui::kDirtyConstraints | ui::kDirtyTransforms))
                solveAssembly();

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
                    if (f.mesh && f.mesh->valid())
                        m_renderer.drawMesh(*f.mesh, f.worldMatrix);
                }
            }

            m_imgui.endFrame();
            m_renderer.endFrame();
        }
    }

    void Application::onExtrude()
    {
        const std::string& paramExpr = m_sketchView.pendingExtrudeParam();
        double height = 1.0;
        if (!m_params.resolve(paramExpr, height))
        {
            MACAD_LOG_WARN("Extrude: cannot resolve '{}' — not a number or known parameter",
                           paramExpr);
            return;
        }

        Feature f;
        f.kind        = FeatureKind::Extrude;
        f.sketchSnap  = m_sketchView.sketch();        // snapshot
        f.param       = paramExpr;
        f.name        = "Extrude " + std::to_string(m_features.size() + 1);
        f.mesh        = std::make_unique<render::Mesh>();

        if (!rebuildFeature(f))
            return;

        m_features.push_back(std::move(f));
        MACAD_LOG_INFO("Extrude done (param='{}', value={:.3f})", paramExpr, height);
    }

    void Application::onRevolve()
    {
        const std::string& paramExpr = m_sketchView.pendingRevolveParam();
        double angle = 360.0;
        if (!m_params.resolve(paramExpr, angle))
        {
            MACAD_LOG_WARN("Revolve: cannot resolve '{}' — not a number or known parameter",
                           paramExpr);
            return;
        }

        Feature f;
        f.kind          = FeatureKind::Revolve;
        f.sketchSnap    = m_sketchView.sketch();
        f.param         = paramExpr;
        f.revolveAroundV = m_sketchView.pendingRevolveAroundV();
        f.name          = "Revolve " + std::to_string(m_features.size() + 1);
        f.mesh          = std::make_unique<render::Mesh>();

        if (!rebuildFeature(f))
            return;

        m_features.push_back(std::move(f));
        MACAD_LOG_INFO("Revolve done (param='{}', value={:.1f}°)", paramExpr, angle);
    }

    bool Application::rebuildFeature(Feature& f)
    {
        double paramVal = 1.0;
        if (!m_params.resolve(f.param, paramVal))
        {
            MACAD_LOG_WARN("rebuildFeature '{}': cannot resolve param '{}'",
                           f.name, f.param);
            return false;
        }

        const sketch::SketchPlane& plane = f.sketchSnap.plane();
        const geometry::Shape face = geometry::SketchToShape::buildFace(f.sketchSnap);
        if (face.isNull())
        {
            MACAD_LOG_WARN("rebuildFeature '{}': no closed profile in stored sketch",
                           f.name);
            return false;
        }

        geometry::Shape solid;
        if (f.kind == FeatureKind::Extrude)
            solid = geometry::SketchToShape::extrude(face, plane, paramVal);
        else
            solid = geometry::SketchToShape::revolve(face, plane, f.revolveAroundV, paramVal);

        if (solid.isNull())
        {
            MACAD_LOG_ERROR("rebuildFeature '{}': geometry failed", f.name);
            return false;
        }

        const MeshData data = geometry::Tessellator::Tessellate(solid, 0.05);
        if (data.vertexCount() == 0)
        {
            MACAD_LOG_ERROR("rebuildFeature '{}': empty tessellation", f.name);
            return false;
        }

        if (!f.mesh) f.mesh = std::make_unique<render::Mesh>();
        f.mesh->upload(data);

        // Store local-space AABB for the assembly solver.
        auto [mn, mx] = data.computeAABB();
        f.aabbMin = mn;
        f.aabbMax = mx;

        // Rebuild world matrix from (possibly updated) transform.
        updateTransform(f);

        m_stats.vertexCount   = static_cast<std::uint32_t>(data.vertexCount());
        m_stats.triangleCount = static_cast<std::uint32_t>(data.triangleCount());
        return true;
    }

    void Application::recompute()
    {
        MACAD_LOG_INFO("Recomputing {} feature(s)...", m_features.size());
        for (Feature& f : m_features)
            rebuildFeature(f);
        solveAssembly();
    }

    // ---- M5: transforms and assembly solver --------------------------------

    glm::mat4 Application::buildMatrix(const FeatureTransform& xf) const
    {
        auto res = [&](const std::string& s, double def) {
            double v = def;
            m_params.resolve(s, v);
            return static_cast<float>(v);
        };

        const float tx = res(xf.tx, 0.0), ty = res(xf.ty, 0.0), tz = res(xf.tz, 0.0);
        const float rx = res(xf.rx, 0.0), ry = res(xf.ry, 0.0), rz = res(xf.rz, 0.0);

        glm::mat4 T  = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz));
        glm::mat4 Rx = glm::rotate(glm::mat4(1.0f), glm::radians(rx), glm::vec3(1,0,0));
        glm::mat4 Ry = glm::rotate(glm::mat4(1.0f), glm::radians(ry), glm::vec3(0,1,0));
        glm::mat4 Rz = glm::rotate(glm::mat4(1.0f), glm::radians(rz), glm::vec3(0,0,1));
        return T * Rz * Ry * Rx;   // TRS order
    }

    void Application::updateTransform(Feature& f)
    {
        f.worldMatrix = buildMatrix(f.xform);
    }

    void Application::solveAssembly()
    {
        // One forward-pass iteration (enough for non-cyclic constraint graphs).
        for (const AsmConstraint& c : m_constraints)
        {
            const int ai = c.featureA, bi = c.featureB;
            if (ai < 0 || bi < 0 ||
                ai >= static_cast<int>(m_features.size()) ||
                bi >= static_cast<int>(m_features.size()))
                continue;

            Feature& A = m_features[ai];
            Feature& B = m_features[bi];

            // Helper: resolve a FeatureTransform field as a double.
            auto res = [&](const std::string& s, double def = 0.0) {
                double v = def;
                m_params.resolve(s, v);
                return v;
            };

            double val = 0.0;
            m_params.resolve(c.value, val);

            switch (c.kind)
            {
            case AsmConstraintKind::ZStack: {
                // B.tz = A.tz + A.local_height (AABB extent in Z)
                const double atz    = res(A.xform.tz);
                const double height = static_cast<double>(A.aabbMax.z - A.aabbMin.z);
                B.xform.tz = std::to_string(atz + height);
                break;
            }
            case AsmConstraintKind::XDistance:
                B.xform.tx = std::to_string(res(A.xform.tx) + val);
                break;
            case AsmConstraintKind::YDistance:
                B.xform.ty = std::to_string(res(A.xform.ty) + val);
                break;
            case AsmConstraintKind::ZDistance:
                B.xform.tz = std::to_string(res(A.xform.tz) + val);
                break;
            case AsmConstraintKind::AlignX:
                B.xform.tx = A.xform.tx;
                break;
            case AsmConstraintKind::AlignY:
                B.xform.ty = A.xform.ty;
                break;
            case AsmConstraintKind::AlignZ:
                B.xform.tz = A.xform.tz;
                break;
            }

            updateTransform(B);
        }
    }

    void Application::shutdown() {
        if (m_window) {
            // Unload plugins first: dynamic plugin commands point into DLL code
            // that must stay mapped until those commands are destroyed.
            m_registry.unloadAll();
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
