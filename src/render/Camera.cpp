#include "render/Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

namespace macad::render {

    namespace 
    {
        constexpr float kPi = 3.14159265358979323846f;
        constexpr float kPitchLimit = kPi * 0.5f - 0.01f;
    } // namespace

    Camera::Camera() = default;

    void Camera::orbit(float dxPixels, float dyPixels) 
    {
        const float speed = 0.005f;
        m_yaw += dxPixels * speed;
        m_pitch = std::clamp(m_pitch + dyPixels * speed, -kPitchLimit, kPitchLimit);
    }

    void Camera::pan(float dxPixels, float dyPixels)
    {
        // Move target in the camera's right/up plane, scaled by distance.
        const mat4 v = viewMatrix();
        const vec3 right{ v[0][0], v[1][0], v[2][0] };
        const vec3 up{ v[0][1], v[1][1], v[2][1] };
        const float scale = m_distance * 0.0015f;
        m_target += (-dxPixels * right + dyPixels * up) * scale;
    }

    void Camera::dolly(float wheelDelta)
    {
        m_distance *= std::pow(0.9f, wheelDelta);
        m_distance = std::clamp(m_distance, 0.05f, 1000.0f);
    }

    void Camera::setViewport(int width, int height) 
    {
        m_width = std::max(1, width);
        m_height = std::max(1, height);
    }

    vec3 Camera::eyePosition() const
    {
        const float cp = std::cos(m_pitch);
        const vec3 dir{ cp * std::cos(m_yaw), std::sin(m_pitch), cp * std::sin(m_yaw) };
        return m_target + dir * m_distance;
    }

    mat4 Camera::viewMatrix() const 
    {
        return glm::lookAt(eyePosition(), m_target, vec3(0.0f, 1.0f, 0.0f));
    }

    mat4 Camera::projMatrix() const 
    {
        const float aspect = static_cast<float>(m_width) / static_cast<float>(m_height);
        return glm::perspective(m_fovY, aspect, 0.05f, 5000.0f);
    }

}  
