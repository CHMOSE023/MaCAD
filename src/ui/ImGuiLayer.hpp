#pragma once

// Owns ImGui context creation and the GLFW + bgfx backends. The app calls
// init() once, then beginFrame()/endFrame() around UI building each frame.

struct GLFWwindow;

namespace macad::ui {

    class ImGuiLayer {
    public:
        ImGuiLayer() = default;
        ~ImGuiLayer();

        bool init(GLFWwindow* window);
        void shutdown();

        // Starts a new ImGui frame (call before building panels).
        void beginFrame();
        // Renders ImGui draw data through the bgfx backend (call after panels).
        void endFrame();

    private:
        bool m_initialized{ false };
    };

} // namespace macad::ui
