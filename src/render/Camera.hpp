#pragma once

// Orbit camera: rotates around a target point, with pan and dolly (zoom).
// Produces view/projection matrices for bgfx::setViewTransform.

#include "core/Types.hpp"

namespace macad::render {

    class Camera {
    public:
        Camera();

        // Mouse-driven controls (deltas in pixels / wheel notches).
        void orbit(float dxPixels, float dyPixels);
        void pan(float dxPixels, float dyPixels);
        void dolly(float wheelDelta);

        void setViewport(int width, int height);

        mat4 viewMatrix() const;
        mat4 projMatrix() const;
        vec3 eyePosition() const;

    private:
        vec3  m_target{ 0.0f, 0.0f, 0.0f };
        float m_distance{ 6.0f };
        float m_yaw{ 0.6f };     // radians, around world up
        float m_pitch{ 0.5f };   // radians, clamped away from poles
        float m_fovY{ 0.9f };    // radians
        int   m_width{ 1280 };
        int   m_height{ 720 };
    };

} // namespace macad::render
