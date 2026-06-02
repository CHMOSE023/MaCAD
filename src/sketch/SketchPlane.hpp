#pragma once

// Maps 2D sketch coordinates (u, v) onto a plane embedded in 3D world space and
// back. The constraint model lives entirely in 2D; the plane is only used for
// rendering the sketch in the 3D viewport and for turning a picked world point
// (from a screen ray) back into sketch coordinates.
//
// For M2 the default plane is the world XY plane (origin, u = +X, v = +Y,
// normal = +Z). The type is general so later milestones can sketch on an
// arbitrary face/datum without touching callers.

#include "core/Types.hpp"

namespace macad::sketch {

    class SketchPlane {
    public:
        SketchPlane() = default;
        SketchPlane(const vec3& origin, const vec3& uAxis, const vec3& vAxis)
            : m_origin(origin), m_u(glm::normalize(uAxis)), m_v(glm::normalize(vAxis)) {}

        // 2D sketch point -> 3D world point.
        vec3 to3d(const vec2& uv) const { return m_origin + m_u * uv.x + m_v * uv.y; }

        // 3D world point -> 2D sketch coords (orthogonal projection onto plane).
        vec2 to2d(const vec3& p) const {
            const vec3 d = p - m_origin;
            return { glm::dot(d, m_u), glm::dot(d, m_v) };
        }

        const vec3& origin() const { return m_origin; }
        const vec3& uAxis() const { return m_u; }
        const vec3& vAxis() const { return m_v; }
        vec3 normal() const { return glm::normalize(glm::cross(m_u, m_v)); }

    private:
        vec3 m_origin{ 0.0f, 0.0f, 0.0f };
        vec3 m_u{ 1.0f, 0.0f, 0.0f };
        vec3 m_v{ 0.0f, 1.0f, 0.0f };
    };

} // namespace macad::sketch
