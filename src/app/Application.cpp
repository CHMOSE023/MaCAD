#include "app/Application.hpp"

#include "core/Log.hpp"
#include "geometry/Primitives.hpp"
#include "geometry/Tessellator.hpp"
#include "plugin/ICommand.hpp"

#include <GLFW/glfw3.h>
#ifdef _WIN32
#include <GLFW/glfw3native.h>
#endif

#include <imgui.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <functional>
#include <memory>

namespace macad::app {

    namespace {

        // Adapts a lambda into an ICommand so the app can register built-ins without a
        // bespoke class per action. Same mechanism plugins will use later.
        class FunctionCommand : public ICommand {
        public:
            FunctionCommand(std::string id, std::string label, std::function<void()> fn)
                : m_id(std::move(id)), m_label(std::move(label)), m_fn(std::move(fn)) {
            }

            std::string id() const override { return m_id; }
            std::string label() const override { return m_label; }
            void execute() override { if (m_fn) m_fn(); }

        private:
            std::string m_id;
            std::string m_label;
            std::function<void()> m_fn;
        };

    } // namespace

    Application::~Application() { shutdown(); }

    bool Application::init(int width, int height) {
        Log::init();
        MACAD_LOG_INFO("MaCAD starting up");

        if (!glfwInit()) {
            MACAD_LOG_ERROR("glfwInit failed");
            return false;
        }

        // bgfx owns rendering: do not create any GL/GLES context.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(width, height, "MaCAD", nullptr, nullptr);
        if (!m_window) {
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

        if (!m_renderer.init(native, width, height)) {
            return false;
        }
        m_camera.setViewport(width, height);

        if (!m_imgui.init(m_window)) {
            return false;
        }

        registerBuiltinCommands();
        rebuildBox(m_boxDims[0], m_boxDims[1], m_boxDims[2]);

        m_stats.backend = m_renderer.backendName();
        m_running = true;
        return true;
    }

    void Application::registerBuiltinCommands() {
        // Demonstrates the command/plugin contract with a built-in, in-process
        // command. A real plugin would register the same way via registerWith().
        m_registry.registerCommand(std::make_shared<FunctionCommand>(
            "macad.geometry.createBox", "Create Box", [this] {
                // Cycle through a couple of sizes to prove the geometry->render
                // round-trip re-runs on demand.
                m_boxDims[0] = (m_boxDims[0] >= 3.0) ? 1.0 : m_boxDims[0] + 0.5;
                rebuildBox(m_boxDims[0], m_boxDims[1], m_boxDims[2]);
            }));

        // M2: enter/leave the 2D sketch editor. The SketchView owns its sketch
        // and drives editing entirely through ImGui overlays + panels.
        m_registry.registerCommand(std::make_shared<FunctionCommand>(
            "macad.sketch.toggle", "Sketch", [this] { m_sketchView.toggle(); }));
    }

    void Application::rebuildBox(double dx, double dy, double dz) {
        const geometry::Shape shape = geometry::Primitives::MakeBox(dx, dy, dz);
        const MeshData data = geometry::Tessellator::Tessellate(shape, 0.05);
        m_mesh.upload(data);
        m_stats.vertexCount = static_cast<std::uint32_t>(data.vertexCount());
        m_stats.triangleCount = static_cast<std::uint32_t>(data.triangleCount());
    }

    void Application::handleCameraInput() {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }
        // In sketch mode the left button drives the sketch (draw/drag/select), so
        // orbit moves to the right button and pan to the middle button; outside
        // sketch mode the left button orbits as before.
        const bool sketching = m_sketchView.active();
        if (!sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            m_camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }
        if (sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            m_camera.orbit(io.MouseDelta.x, io.MouseDelta.y);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            (!sketching && ImGui::IsMouseDragging(ImGuiMouseButton_Right))) {
            m_camera.pan(io.MouseDelta.x, io.MouseDelta.y);
        }
        if (io.MouseWheel != 0.0f) {
            m_camera.dolly(io.MouseWheel);
        }
    }

    void Application::syncFramebufferSize() {
        int w = 0, h = 0;
        glfwGetFramebufferSize(m_window, &w, &h);
        if (w > 0 && h > 0 && (w != m_renderer.width() || h != m_renderer.height())) {
            m_renderer.reset(w, h);
            m_camera.setViewport(w, h);
        }
    }

    void Application::run() {
        double lastTime = glfwGetTime();

        while (m_running && !glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            syncFramebufferSize();

            const double now = glfwGetTime();
            const double dt = now - lastTime;
            lastTime = now;
            m_stats.fps = dt > 0.0 ? 1.0 / dt : 0.0;

            m_imgui.beginFrame();
            handleCameraInput();
            ui::Panels::draw(m_registry, m_stats);

            // Sketch overlay needs the camera's view-projection to map the 2D
            // plane to screen space. The same matrix drives picking/dragging.
            const glm::mat4 viewProj = m_camera.projMatrix() * m_camera.viewMatrix();
            m_sketchView.update(viewProj);
            m_sketchView.drawPanels();

            m_renderer.beginFrame(m_camera);
            if (!m_sketchView.active()) {
                m_renderer.drawMesh(m_mesh, glm::mat4(1.0f));
            }

            m_imgui.endFrame();
            m_renderer.endFrame();
        }
    }

    void Application::shutdown() {
        if (m_window) {
            m_imgui.shutdown();
            m_mesh.destroy();      // free GPU buffers while bgfx is still alive
            m_renderer.shutdown(); // calls bgfx::shutdown()
            glfwDestroyWindow(m_window);
            m_window = nullptr;
            glfwTerminate();
            MACAD_LOG_INFO("MaCAD shut down");
        }
        m_running = false;
    }

}  
