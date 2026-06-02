#pragma once

// Owns the bgfx device, the main view, and the mesh shader program. The app
// layer drives it: init() once, then per-frame beginFrame -> drawMesh* ->
// endFrame. bgfx types are hidden from this header.

#include "core/Types.hpp"

#include <cstdint>

namespace macad::render {

    class Mesh;
    class Camera;

    // Platform window handles, filled by the app from GLFW. Keeping this a plain
    // struct avoids leaking bgfx/GLFW headers into the renderer's interface.
    struct NativeWindow {
        void* nwh{ nullptr }; // native window handle (HWND on Windows)
        void* ndt{ nullptr }; // native display type (nullptr on Windows)
    };

    class Renderer {
    public:
        Renderer() = default;
        ~Renderer();

        bool init(const NativeWindow& window, int width, int height);
        void shutdown();

        void reset(int width, int height);

        // Sets view/proj from the camera and clears the backbuffer.
        void beginFrame(const Camera& camera);
        // Draws a mesh with the given model transform.
        void drawMesh(const Mesh& mesh, const mat4& model);
        // Advances bgfx to the next frame.
        void endFrame();

        // Backend name for diagnostics (e.g. "Direct3D11", "Vulkan").
        const char* backendName() const;

        int width() const { return m_width; }
        int height() const { return m_height; }

    private:
        bool createProgram();

        std::uint16_t m_program{ 0xffff }; // bgfx::ProgramHandle idx
        int m_width{ 0 };
        int m_height{ 0 };
        bool m_initialized{ false };
    };

} // namespace macad::render
